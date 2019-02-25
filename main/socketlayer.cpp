// Copyright (c) 2001-2008, Scalable Network Technologies, Inc.  All Rights Reserved.
//                          6100 Center Drive West
//                          Suite 1250 
//                          Los Angeles, CA 90045
//                          sales@scalable-networks.com
//
// This source code is licensed, not sold, and is subject to a written
// license agreement.  Among other things, no portion of this source
// code may be copied, transmitted, disclosed, displayed, distributed,
// translated, used as the basis for a derivative work, or used, in
// whole or in part, for any program or purpose other than its intended
// use in compliance with the license agreement as part of the EXata
// software.  This source code and certain of the algorithms contained
// within it are confidential trade secrets of Scalable Network
// Technologies, Inc. and may not be used as the basis for any other
// software, hardware, product or service.

#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/time.h>
#include <sys/types.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#endif

#include "api.h"
#include "app_util.h"
#include "network_ip.h"
#include "socketlayer.h"
#include "socketlayer_api.h"

#include "ControlMessages.h"

#ifndef ARPHRD_LOOPBACK
#define ARPHRD_LOOPBACK 772
#endif

//#define SL_DEBUG

/** COMMENTS
 1. all addresses and port number are in host order

 */
int SocketLayerFindNextFd(
    Node *node)
{
    int fd;    
    SLData *slData = node->slData;
    
    for(fd = 0; fd < SL_MAX_SOCK_FDS; fd++)
    {
        if(!slData->socketList[fd])
            return fd;
    }

    assert(0);
    return -1;
}

// XXX So if we have more than one open sockets with same port number?
// right now we are returning the same socket
int GetSLSocketFdFromPortNum(
        Node *node,
        unsigned short portNum,
        NodeAddress remoteAddr,
        unsigned short remotePort)
{
    SLData *slData = node->slData;
    SLSocketData *sData;
    int fd, unconnected_fd = -1;

    for(fd=0; fd < SL_MAX_SOCK_FDS; fd++)
    {
        sData = slData->socketList[fd];

        if(sData && (sData->localAddress.sin_port == portNum) &&
            (0 == sData->remoteAddress.sin_addr.s_addr) &&
            (0 == sData->remoteAddress.sin_port) )
        {
            unconnected_fd = fd;
        }
            
        if(sData && (sData->localAddress.sin_port == portNum) &&
            (remoteAddr == sData->remoteAddress.sin_addr.s_addr) &&
            (remotePort == sData->remoteAddress.sin_port) )
            break;
    }

    if(fd == SL_MAX_SOCK_FDS)
    {
        if(unconnected_fd > -1)
            return unconnected_fd;

        return -1;
    }
    return fd;
}

int GetSLSocketFdFromConnectionId(
        Node *node,
        int connId)
{
    SLData *slData = node->slData;
    SLSocketData *sData;
    int fd;

    for(fd=0; fd < SL_MAX_SOCK_FDS; fd++)
    {
        sData = slData->socketList[fd];
        if(sData && 
            (sData->connectionState == SL_SOCKET_CONNECTED ||
                sData->connectionState == SL_SOCKET_CLOSING) &&
            sData->connectionId == connId)
        {
            break;
        }
    }

    if(fd == SL_MAX_SOCK_FDS)
    {
        return -1;
    }
    return fd;
}

/*******************************************
 * Socket related API
 ******************************************/
int SL_socket(
    Node *node, 
    int family, 
    int protocol, 
    int type)
{
    SLSocketData *sData;
    int fd;
    Address myDefaultAddress;

    sData = (SLSocketData *)MEM_malloc(sizeof(SLSocketData));

    if(!sData)
        return -1;

    fd = SocketLayerFindNextFd(node);

    node->slData->socketList[fd] = sData;

    sData->family     = family;
    sData->protocol = protocol;
    sData->type        = type;

    sData->refCount = 1;
    sData->connectionState = SL_SOCKET_UNCONNECTED;
    sData->isNonBlocking = FALSE;

    sData->recvfromFunction    = NULL;
    sData->recvFunction     = NULL;
    sData->listenFunction     = NULL;
    sData->connectFunction     = NULL;
    sData->sendFunction     = NULL;

    sData->acceptBacklogIndex = -1;

    NetworkGetInterfaceInfo(
        node, 0, &myDefaultAddress);
    // IPV6 change here
    sData->localAddress.sin_addr.s_addr = myDefaultAddress.interfaceAddr.ipv4;
    sData->localAddress.sin_port = APP_GetFreePort(node);

    APP_InserInPortTable(node, 
            APP_SOCKET_LAYER, 
            sData->localAddress.sin_port);
    
    sData->remoteAddress.sin_port = 0;
    sData->remoteAddress.sin_addr.s_addr = 0;

    sData->isUpaSocket = FALSE;
    sData->upaRecvBuffer = new queue<Message *>();
    sData->lastBufferRead = FALSE;

    return fd;
}

int SL_ioctl(
    Node *node,
    int fd, 
    unsigned long cmd,
    unsigned long arg)
{
    SLSocketData *sData = node->slData->socketList[fd];

    if(!sData)
    {    
        return -1;
    }

    // Later on check based on CtrlMsg_0x3000 < cmd etc 
    switch(cmd)
    {
        /* APPLICATION layer IOCTLs */
        /* TRANSPORT layer IOCTLs */
        /* NETWORK layer IOCTLs */
        case CtrlMsg_Network_GetRoute:
        {

            break;
        }
        case CtrlMsg_Network_SetRoute:
        {
            break;
        }
        
        /* MAC layer IOCTLs */
        case CtrlMsg_MAC_GetInterfaceList:
        {
            char *dataPtr = (char *)arg;
            int numInterfaces, thisInterfaceIndex;
            SLInterfaceInfo *ifInfo;
            struct sockaddr_in addr;
            NetworkDataIp *ip = (NetworkDataIp *)node->networkData.networkVar;

            numInterfaces = node->numberInterfaces;

            // The first interface is the loopback interface
            ifInfo = (SLInterfaceInfo *)dataPtr;
            dataPtr += sizeof(SLInterfaceInfo);
            
            sprintf(ifInfo->ifname, "lo");

            ifInfo->ifaddr.sin_family = AF_INET;
            ifInfo->ifaddr.sin_addr.s_addr = 0x100007f;
            ifInfo->netmask.sin_family = AF_INET;
            ifInfo->netmask.sin_addr.s_addr = htonl(0xFF000000);

            ifInfo->phyaddr.sa_family = ARPHRD_LOOPBACK;

#ifdef _WIN32
            ifInfo->flags = 0x1 | 0x40 | 0x8;
#else
            ifInfo->flags = (IFF_UP | IFF_RUNNING | IFF_LOOPBACK);
#endif
            ifInfo->ifindex = 0;
            ifInfo->metric = 1;
            ifInfo->mtu = 16436;

            // Now fill the other interfaces
            for(thisInterfaceIndex=1; thisInterfaceIndex <= numInterfaces; thisInterfaceIndex++)
            {
                ifInfo = (SLInterfaceInfo *)dataPtr;
                dataPtr += sizeof(SLInterfaceInfo);

                sprintf(ifInfo->ifname, "qln%d", thisInterfaceIndex - 1);
                ifInfo->ifaddr.sin_family = AF_INET;
                ifInfo->ifaddr.sin_addr.s_addr = htonl(ip->interfaceInfo[thisInterfaceIndex - 1]->ipAddress);

                ifInfo->netmask.sin_family = AF_INET;
                ifInfo->netmask.sin_addr.s_addr = htonl(NetworkIpGetInterfaceSubnetMask(node, thisInterfaceIndex - 1));

                ifInfo->phyaddr.sa_family = 1; //ARPHRD_ETHER;
                memcpy(&ifInfo->phyaddr.sa_data, "98989898", 8);
                

#ifdef _WIN32
                ifInfo->flags = 0x1 | 0x40 ;
#else
                ifInfo->flags = IFF_UP | IFF_RUNNING;
#endif
                ifInfo->ifindex = thisInterfaceIndex;
                ifInfo->metric = 1;
                ifInfo->mtu = 1500;
            }

            break;
        }
        case CtrlMsg_MAC_GetInterfaceAddress:
        {
            struct sockaddr_in *addr = (struct sockaddr_in *)arg;
            memcpy(addr, &sData->localAddress, sizeof(struct sockaddr));
            break;
        }
        case CtrlMsg_MAC_SetInterfaceAddress:
        {
            struct sockaddr_in *addr = (struct sockaddr_in *)arg;
            memcpy(&sData->localAddress, addr, sizeof(struct sockaddr));
            break;
        }
        case CtrlMsg_MAC_GetInterfaceName:
        {
            break;
        }
        case CtrlMsg_MAC_SetInterfaceName:
        {
            break;
        }
        case CtrlMsg_MAC_SetBroadcastAddress:
        {
            break;
        }
        case CtrlMsg_MAC_GetNetmask:
        {
            break;
        }
        case CtrlMsg_MAC_SetNetmask:
        {
            break;
        }

        
        /* PHY layer IOCTLs */

        case CtrlMsg_PHY_SetInterfaceMTU:
        {
            break;
        }
        case CtrlMsg_PHY_GetInterfaceMTU:
        {
            break;
        }
        case CtrlMsg_PHY_SetTxPower:
        {
            //PHY_SetTxPower(node, phyIndex, *(double *)arg);
            break;
        }
        case CtrlMsg_PHY_GetTxPower:
        {
            //PHY_GetTxPower(node, phyIndex, (double *)arg);
            break;
        }
        case CtrlMsg_PHY_SetDataRate:
        {
            //PHY_SetDataRate(node, phyIndex, *(int *)arg);
            break;
        }
        case CtrlMsg_PHY_GetDataRate:
        {
            //int rate = PHY_GetTxDataRate(node, phyIndex);
            //*(int *)arg = rate;
            break;
        }
        case CtrlMsg_PHY_SetTxChannel:
        {
            break;
        }
        case CtrlMsg_PHY_GetTxChannel:
        {
            break;
        }
        case CtrlMsg_PHY_SetHWAddress:
        {
            break;
        }
        case CtrlMsg_PHY_GetHWAddress:
        {
            break;
        }
        case CtrlMsg_PHY_SetInterfaceFlags:
        {
            break;
        }
        case CtrlMsg_PHY_GetInterfaceFlags:
        {
            break;
        }
        case CtrlMsg_PHY_SetInterfaceMetric:
        {
            break;
        }
        case CtrlMsg_PHY_GetInterfaceMetric:
        {
            break;
        }

        /* UPA layer IOCTLs */
        case CtrlMsg_UPA_SetUpaSocket:
        {
            sData->isUpaSocket = TRUE;
            break;
        }
        case CtrlMsg_UPA_SetPhysicalAddress:
        {
            struct sockaddr_in *addr = (struct sockaddr_in *)arg;
            memcpy(&sData->upaPhysicalAddr, addr, sizeof(struct sockaddr));
            break;
        }
        case CtrlMsg_UPA_GetPhysicalAddress:
        {
            struct sockaddr_in *addr = (struct sockaddr_in *)arg;
            memcpy(addr, &sData->upaPhysicalAddr, sizeof(struct sockaddr));
            break;
        }
        default:
            ERROR_ReportError("Undefined Control Message ioctl\n");
    }

    return 0;

}


int SL_sendto(
    Node *node,
    int fd,
    void *data,
    int dataSize,
    struct sockaddr *addr,
    int virtualLength)
{
    SLSocketData *sData = node->slData->socketList[fd];

    if(!sData)
        return -1;
    
    switch(sData->family)
    {
        case AF_INET:
        {
            struct sockaddr_in *addr_in = (struct sockaddr_in *)addr;

#ifdef SL_DEBUG
            printf("\t[SL] %d SL sending to <%x:%d> from <%x:%d>\n", node->nodeId,
                addr_in->sin_addr.s_addr,
                addr_in->sin_port,
                sData->localAddress.sin_addr.s_addr,
                sData->localAddress.sin_port);
#endif

            APP_UdpSendNewData(
                node,
                (AppType)addr_in->sin_port,
                sData->localAddress.sin_addr.s_addr,
                (short)sData->localAddress.sin_port,
                addr_in->sin_addr.s_addr,
                (char *)data,
                dataSize,
                (clocktype)0,
                TRACE_SOCKETLAYER,
                TRUE);
                
            break;
        }
        default:
            ERROR_ReportError("--");
    }
    return 0; 
}


int SL_send(
    Node *node,
    int fd, 
    void *data, 
    int dataSize,
    int virtualLength)
{
    SLSocketData *sData = node->slData->socketList[fd];

    if(!sData)
        return -1;
    
    if(sData->connectionState != SL_SOCKET_CONNECTED)
        return -1;
    
    if(sData->type == SOCK_DGRAM)
    {
        return SL_sendto(node, fd, data, dataSize, 
            (struct sockaddr *)&sData->remoteAddress, virtualLength);
    }
    else if(sData->type == SOCK_STREAM)
    {
        APP_TcpSendData(
                node,
                sData->connectionId,
                (char *)data,
                dataSize,
                TRACE_SOCKETLAYER,
                TRUE);
    }
    else
    {
        assert(0);
    }
    return -1;
}

int SL_read(
    Node *node,
    int fd, 
    void *buf, 
    int n)
{
    SLSocketData *sData = node->slData->socketList[fd];

    if(!sData)
        return -1;

    return 0;
}




int SL_recvfrom(
    Node *node,
    int fd, 
    void *buf, 
    int n,
    int flags, 
    struct sockaddr *addr,
    int *addr_len)
{
    SLSocketData *sData = node->slData->socketList[fd];

    if(!sData)
        return -1;
    
    if(!sData->isNonBlocking)
        ERROR_ReportError("SL_recvfrom can be called on non-blocking sockets only\n");
    
    return 0;
}


int SL_recv(
    Node *node,
    int fd, 
    void **buf, 
    int n,
    int flags)
{
    SLSocketData *sData = node->slData->socketList[fd];

    if(!sData)
        return -1;
    
    if(sData->lastBufferRead)
    {
        MESSAGE_Free(node, sData->upaRecvBuffer->front());
        sData->upaRecvBuffer->pop();
    }

    if(sData->upaRecvBuffer->size() == 0)
    {
        sData->lastBufferRead = FALSE;
        return -1;
    }

    *buf = MESSAGE_ReturnPacket(sData->upaRecvBuffer->front());
    sData->lastBufferRead = TRUE;

    return MESSAGE_ReturnPacketSize(sData->upaRecvBuffer->front());
}



int SL_write(
    Node *node,
    int fd, 
    void *buf, 
    int n,
    int virtualLength)
{
    SLSocketData *sData = node->slData->socketList[fd];

    if(!sData)
        return -1;
    
    return 0;
}


int SL_bind(
    Node *node,
    int fd,
    struct sockaddr *addr,
    int addr_len)
{
    SLSocketData *sData;
    struct sockaddr_in *addr_in = (struct sockaddr_in *)addr;

    sData = node->slData->socketList[fd];
    if(!sData)
        return -1;
    
    if(addr_in->sin_port == 0)
    {
        return 0;
    }

    if(!APP_IsFreePort(node, addr_in->sin_port))
    {
#ifdef SL_DEBUG
        printf("PORT NOT FREE: %d at %d\n", addr_in->sin_port, node->nodeId);
#endif 
    }

    APP_RemoveFromPortTable(node, sData->localAddress.sin_port);

    memcpy(&sData->localAddress, addr, sizeof(struct sockaddr));

    APP_InserInPortTable(node, 
            APP_SOCKET_LAYER, 
            sData->localAddress.sin_port);

    return 0;
}


int SL_connect(
    Node *node,
    int fd,
    struct sockaddr *addr,
    int addr_len)
{
    SLSocketData *sData = node->slData->socketList[fd];

    if(!sData)
        return -1;


    switch(sData->family)
    {
        case AF_INET:
        {
            struct sockaddr_in *addr_in = (struct sockaddr_in *)addr;

            if(sData->type == SOCK_DGRAM)
            {
                sData->connectionState = SL_SOCKET_CONNECTED;
                memcpy(&sData->remoteAddress, addr, sizeof(struct sockaddr));
            }
            else if(sData->type == SOCK_STREAM)
            {
                struct sockaddr_in *addr_in = (struct sockaddr_in *)addr;

                assert(sData->connectFunction);

                sData->uniqueId = node->appData.uniqueId++;

                APP_TcpOpenConnection(
                        node,
                        (AppType)sData->localAddress.sin_port,
                        sData->localAddress.sin_addr.s_addr,
                        (short)sData->localAddress.sin_port,
                        addr_in->sin_addr.s_addr,
                        (short)addr_in->sin_port,
                        sData->uniqueId, // XXX
                        0);

            }
            else
            {
                ERROR_ReportError("Unknown protocol type\n");
            }
            break;
        }
        default:
        ERROR_ReportError("--");
    }
    return 0;
}


int SL_accept(
    Node *node,
    int fd,
    struct sockaddr *addr,
    int *addr_len)
{
    SLSocketData *sData = node->slData->socketList[fd], *newSData;
    int newFd;

    if(!sData)
        return -1;

    assert(sData);

    newFd = SL_socket(node, sData->family, sData->protocol, sData->type);
    assert(newFd >= 0);

    newSData = node->slData->socketList[newFd];
    assert(newSData);

    memcpy(addr, &sData->acceptBacklog[sData->acceptBacklogIndex].remoteAddr, sizeof(struct sockaddr));
    memcpy(&newSData->remoteAddress, addr, sizeof(struct sockaddr));

    newSData->connectionState = SL_SOCKET_CONNECTED;
    newSData->connectionId    = sData->acceptBacklog[sData->acceptBacklogIndex].connectionId;

    newSData->recvfromFunction    = sData->recvfromFunction;
    newSData->recvFunction         = sData->recvFunction;
    newSData->connectFunction     = sData->connectFunction;
    newSData->listenFunction     = sData->listenFunction;
    newSData->sendFunction        = sData->sendFunction;

    memcpy(&newSData->localAddress, &sData->localAddress, sizeof(struct sockaddr));
    memset(&newSData->upaPhysicalAddr, 0, sizeof(struct sockaddr));
    sData->acceptBacklogIndex --;

    return newFd;
}


#ifndef _WIN32
int SL_poll(
    Node *node,
    struct pollfd *fds,
    nfds_t nfds,
    int timeout)
{
    
}
#endif


int SL_getsockname(
    Node *node,
    int fd,
    struct sockaddr *addr,
    int *addr_len)
{
    SLSocketData *sData = node->slData->socketList[fd];

    if(!sData)
        return -1;

    if(addr == NULL)
        return -1;
    
    memcpy(addr, &sData->localAddress, sizeof(struct sockaddr));
    *addr_len = sizeof(struct sockaddr);
    return 0;
}


int SL_getpeername(
    Node *node,
    int fd,
    struct sockaddr *addr,
    int *addr_len)
{
    SLSocketData *sData = node->slData->socketList[fd];

    if(!sData)
        return -1;
    
    switch(sData->family)
    {
        case AF_INET:
        {
            memcpy(addr, &sData->remoteAddress, sizeof(struct sockaddr));
            *addr_len = sizeof(struct sockaddr);
            break;
        }
        default:
            ERROR_ReportError(" ");
    }
    return 0;
}


int SL_listen(
    Node *node,
    int fd,
    int backlog)
{
    SLSocketData *sData = node->slData->socketList[fd];

    if(!sData)
        return -1;

    assert(sData->listenFunction);

    // TODO: check if this is TCP socket

    if(sData->connectionState == SL_SOCKET_LISTENING)
        return 0;

    sData->connectionState = SL_SOCKET_LISTENING;
    APP_TcpServerListen(
            node,
            (AppType)sData->localAddress.sin_port,
            sData->localAddress.sin_addr.s_addr,
            sData->localAddress.sin_port);
    return 0;
}

int SL_close(
    Node *node,
    int fd)
{
    SLSocketData *sData = node->slData->socketList[fd];

    if(!sData)
        return 0;

    sData->refCount--;

    if(sData->refCount)
    {
        return 0;
    }

    if(sData->type == SOCK_DGRAM)
    {
        APP_RemoveFromPortTable(node, sData->localAddress.sin_port);
        node->slData->socketList[fd] = NULL;
        free(sData);
        return 0;
    }

    // XXX default to TCP
    switch(sData->connectionState)
    {
        case SL_SOCKET_UNCONNECTED:
        case SL_SOCKET_LISTENING: /* TODO: Should detach the inpcb */
            {
                APP_RemoveFromPortTable(node, sData->localAddress.sin_port);
                node->slData->socketList[fd] = NULL;
                free(sData);
                break;
            }
        case SL_SOCKET_CONNECTED:
        {
            sData->connectionState = SL_SOCKET_CLOSING;
            APP_TcpCloseConnection(node, sData->connectionId);
            break;
        }
        case SL_SOCKET_CLOSING:
        {
            // If no other socket is using this port number, then release it
            unsigned short portNum = sData->localAddress.sin_port;

            sData->localAddress.sin_port = 0;

            if(GetSLSocketFdFromPortNum(node, portNum, 0, 0) < 0)
                APP_RemoveFromPortTable(node, sData->localAddress.sin_port);

            node->slData->socketList[fd] = NULL;
            free(sData);
            break;
        }
        default:
        {
            assert(0);
        }
    }
    return 0;
}


int SL_getsockopt(
    Node *node,
    int fd,
    int level,
    int optname,
    void *optval,
    int *optlen)
{
    SLSocketData *sData = node->slData->socketList[fd];

    if(!sData)
        return -1;
    return 0;
    
}


int SL_setsockopt(
    Node *node,
    int fd,
    int level,
    int optname,
    void *optval,
    int optlen)
{
    SLSocketData *sData = node->slData->socketList[fd];

    if(!sData)
        return -1;
    return 0;
    
}


int SL_fcntl(
    Node *node,
    int fd, 
    int cmd,
    int arg)
{
    SLSocketData *sData = node->slData->socketList[fd];

    if(!sData)
        return -1;
    
    return 0;
}


int SL_recvmsg(
    Node *node,
    int fd,
    struct msghdr *msg,
    int flags)
{
    SLSocketData *sData = node->slData->socketList[fd];

    if(!sData)
        return -1;
    
    return 0;
}


int SL_select(
    Node *node,
    int nfds,
    fd_set *readfds,
    fd_set *writefds,
    fd_set *exceptfds,
    struct timeval *timeout)
{
    return 0;
    
    return 0;
}

void SL_fork(
    Node *node,
    int fd)
{
    SLSocketData *sData = node->slData->socketList[fd];

    assert(sData);

    sData->refCount++;
}

/*****************************************************
 * SocketLayer
 ****************************************************/

void SocketLayerInit(
    Node *node,
    const NodeInput *nodeInput)
{
    SLData *slData;

    slData = (SLData *)MEM_malloc(sizeof(SLData));
    node->slData = slData;

    for(int i=0; i < SL_MAX_SOCK_FDS; i++)
        slData->socketList[i] = NULL;
        
    slData->nextPortNum = SL_START_PORT_NUM;

}

BOOL SocketLayerProcessEvent(
    Node *node,
    Message *msg)
{
    int fd;
    SLSocketData *sData = NULL;

    switch(msg->eventType)
    {
        case MSG_APP_TimerExpired:
        {
            if(MESSAGE_GetProtocol(msg) != APP_SOCKET_LAYER)
                return FALSE;

            SLTimerData *info = (SLTimerData *)MESSAGE_ReturnInfo(msg);

            info->timerFunction(node, info->data);
            if(info->timerType == SL_TIMER_REPEAT)
                MESSAGE_Send(node, msg, info->interval);
            else
                MESSAGE_Free(node, msg);

            break;
        }
        case MSG_APP_FromTransport:
        {
            // XXX check AF_INET and udp
            UdpToAppRecv *info = (UdpToAppRecv *)MESSAGE_ReturnInfo(msg);
            unsigned short portNum = info->destPort;
            
            fd = GetSLSocketFdFromPortNum(node, 
                        portNum, 
                        info->sourceAddr.interfaceAddr.ipv4, 
                        info->sourcePort);

            if(fd < 0)
                return FALSE;

            sData = node->slData->socketList[fd];
            assert(sData);

            if(sData->recvfromFunction)
            {
                struct sockaddr_in addr;
                addr.sin_family = sData->family;
                addr.sin_addr.s_addr = info->sourceAddr.interfaceAddr.ipv4;
                addr.sin_port = info->sourcePort;

                if(sData->connectionState == SL_SOCKET_UNCONNECTED)
                {
                    sData->recvfromFunction(node, 
                            fd, 
                            MESSAGE_ReturnPacket(msg), 
                            MESSAGE_ReturnPacketSize(msg), 
                            (struct sockaddr *)&addr);
                }
                else if(sData->connectionState == SL_SOCKET_CONNECTED)
                {
                    sData->recvfromFunction(node, 
                            fd, 
                            MESSAGE_ReturnPacket(msg), 
                            MESSAGE_ReturnPacketSize(msg), 
                            NULL);
                }
                else
                {
                    assert(0);
                }

            }
            else if(sData->recvFunction)
            {
                sData->recvFunction(node, 
                            fd, 
                            MESSAGE_ReturnPacket(msg), 
                            MESSAGE_ReturnPacketSize(msg));

            }
            else
            {
#ifdef SL_DEBUG
                printf("[SL] No receive function defined\n");
#endif 
            }

            break;
        }
        case MSG_APP_FromTransListenResult:
        {
            TransportToAppListenResult *result = (TransportToAppListenResult *)    
                            MESSAGE_ReturnInfo(msg);

            unsigned short portNum = result->localPort;

            fd = GetSLSocketFdFromPortNum(node, portNum, 0, 0);
            if(fd < 0)
                return FALSE;

            sData = node->slData->socketList[fd];
            assert(sData);

            //assert(result->connectionId >= 0);
            if(result->connectionId < 0)
            {
#ifdef SL_DEBUG
                printf("[SL %d] ERROR Connection id is %d\n", node->nodeId, result->connectionId);
#endif 
            }

            if(sData->listenFunction)
                sData->listenFunction(node, fd);

            break;
        }
        case MSG_APP_FromTransOpenResult:
        {
            TransportToAppOpenResult *result = (TransportToAppOpenResult *)    
                            MESSAGE_ReturnInfo(msg);
            unsigned short portNum = result->localPort;

            fd = GetSLSocketFdFromPortNum(node, portNum, 0, 0);
            if(fd < 0)
                return FALSE;

            sData = node->slData->socketList[fd];
            assert(sData);

            if(sData->connectionState == SL_SOCKET_LISTENING)
            {
                // this is the server socket 
                if(sData->acceptFunction)
                {
                    if(sData->acceptBacklogIndex < SL_MAX_ACCEPT_BACKLOG - 1)
                    {
                        struct sockaddr_in remoteAddr;

                        sData->acceptBacklogIndex++;
                        
                        remoteAddr.sin_addr.s_addr = result->remoteAddr.interfaceAddr.ipv4;
                        remoteAddr.sin_port = result->remotePort;

                        memcpy(&sData->acceptBacklog[sData->acceptBacklogIndex].remoteAddr,
                                &remoteAddr,
                                sizeof(remoteAddr));

                        sData->acceptBacklog[sData->acceptBacklogIndex].connectionId = 
                                result->connectionId;
                            
                        sData->acceptFunction(node, fd);
                    }
                    else
                    {
#ifdef SL_DEBUG
                        printf("[SL] Not enough room to store incoming listen at node %d\n", node->nodeId);
#endif 
                    }
                        
                }
                else
                {
#ifdef SL_DEBUG
                    printf("[SL] listenFunction not defined\n");
#endif     
                }

            }
            else if(sData->connectionState == SL_SOCKET_UNCONNECTED)
            {
                // this is the client socket
                if(sData->connectFunction)
                {
                    struct sockaddr_in remoteAddr;

                    remoteAddr.sin_addr.s_addr = 
                                result->remoteAddr.interfaceAddr.ipv4, 
                    remoteAddr.sin_port = result->remotePort;

                    sData->connectionState = SL_SOCKET_CONNECTED;
                    sData->connectionId = result->connectionId;

                    memcpy(&sData->remoteAddress, &remoteAddr, sizeof(remoteAddr));

                    sData->connectFunction(node, 
                                fd, 
                                (struct sockaddr *)&remoteAddr);
                }
                else
                {
#ifdef SL_DEBUG
                    printf("[SL] connectFunction not defined\n");
#endif 
                    assert(0);
                }
            }
            else
            {
#ifdef SL_DEBUG
                printf("[SL %d] Incorrect socket connectionstate = %d. fd=%d\n", node->nodeId, sData->connectionState, fd);
#endif     
            }

            break;
        }
        case MSG_APP_FromTransDataReceived:
        {
            TransportToAppDataReceived *result = (TransportToAppDataReceived *)    
                            MESSAGE_ReturnInfo(msg);

            fd = GetSLSocketFdFromConnectionId(node, result->connectionId);
            if(fd < 0)
                return FALSE;

            sData = node->slData->socketList[fd];
            assert(sData);

            /* UPA Specific: if we got data from client, BEFORE the upa client
                can register the newly accepted socket. */
            if(sData->isUpaSocket)
            {
                struct sockaddr_in *addr = 
                    (struct sockaddr_in *)&sData->upaPhysicalAddr;
                if(addr->sin_addr.s_addr == 0)
                {
                    sData->upaRecvBuffer->push(msg);
                    return TRUE;
                }
            }
            else
            {
                assert(0);
            }
            
            if(sData->recvfromFunction)
            {
#if 0    
                struct sockaddr_in addr;
                addr.sin_family = sData->family;
                addr.sin_addr.s_addr = sData->remoteAddress.sin_addr.s_addr;
                addr.sin_port = sData->remoteAddress.sin_port;
#endif

                sData->recvfromFunction(node, 
                            fd, 
                            MESSAGE_ReturnPacket(msg), 
                            MESSAGE_ReturnPacketSize(msg), 
                            NULL);

            }
            else if(sData->recvFunction)
            {
                sData->recvFunction(node, 
                            fd, 
                            MESSAGE_ReturnPacket(msg), 
                            MESSAGE_ReturnPacketSize(msg));

            }
            else
            {
#ifdef SL_DEBUG
                printf("[SL] No receive function defined\n");
#endif 
            }

            break;
        }
        case MSG_APP_FromTransDataSent:
        {
            TransportToAppDataSent *result = (TransportToAppDataSent *)    
                            MESSAGE_ReturnInfo(msg);

            fd = GetSLSocketFdFromConnectionId(node, result->connectionId);
            if(fd < 0)
                return FALSE;

            sData = node->slData->socketList[fd];
            assert(sData);

            if(sData->sendFunction)
            {
                sData->sendFunction(node, fd, result->length);
            }

            break;
        }
        case MSG_APP_FromTransCloseResult:
        {
            TransportToAppCloseResult *result = (TransportToAppCloseResult *)
                            MESSAGE_ReturnInfo(msg);

            fd = GetSLSocketFdFromConnectionId(node, result->connectionId);
            if(fd < 0)
                return FALSE;
            
            sData = node->slData->socketList[fd];
            
            if(sData->connectionState == SL_SOCKET_CLOSING)
            {
                SL_close(node, fd);
            }
            else
            {
                //sData->connectionState = SL_SOCKET_CLOSING;
                //APP_TcpCloseConnection(node, sData->connectionId);
                if(sData->isUpaSocket)
                {
                    struct sockaddr_in *addr = 
                        (struct sockaddr_in *)&sData->upaPhysicalAddr;
                    if(addr->sin_addr.s_addr == 0)
                    {
                        sData->upaRecvBuffer->push(msg);
                        return TRUE;
                    }
                    else
                    {
                        sData->recvfromFunction(node, fd, NULL, 0, NULL);
                    }

                    break;
                }

                if(sData->recvfromFunction)
                {
                    sData->recvfromFunction(node, fd, NULL, 0, NULL);
                }
                else if(sData->recvFunction)
                {
                    sData->recvFunction(node, fd, NULL, 0);
                }
                else
                {
                    assert(0);
                }
            }
            break;
        }
        default:
        {
            return FALSE;
        }
    }

    MESSAGE_Free(node, msg);

    return TRUE;
}

void SocketLayerSetTimer(
    Node *node,
    SLTimerType timerType,
    struct timeval *interval,
    void (*timerFunction)(Node *, void *),
    void *data)
{
    Message *msg;
    SLTimerData *info;

    msg = MESSAGE_Alloc(node, 
                APP_LAYER,
                APP_SOCKET_LAYER,
                MSG_APP_TimerExpired);
    
    MESSAGE_InfoAlloc(node, msg, sizeof(data) + sizeof(timerFunction));
    info = (SLTimerData *)MESSAGE_ReturnInfo(msg);

    info->data = data;
    info->timerFunction = timerFunction;
    info->timerType = timerType;
    info->interval = interval->tv_sec * SECOND + interval->tv_usec * MICRO_SECOND;

    MESSAGE_Send(node, msg, info->interval);
}


void SocketLayerRegisterCallbacks(
    Node *node,
    int fd,
    void (*recvfromFunction)(Node *, int, char *, int, struct sockaddr *),
    void (*recvFunction)(Node *, int, char *, int),
    void (*listenFunction)(Node *, int),
    void (*acceptFunction)(Node *, int),
    void (*connectFunction)(Node *, int, struct sockaddr *),
    void (*sendFunction)(Node *, int, int))
{
    SLSocketData *sData = node->slData->socketList[fd];

    assert(sData);

    if(recvfromFunction)
        sData->recvfromFunction    = recvfromFunction;
    
    if(recvFunction)
        sData->recvFunction     = recvFunction;

    if(listenFunction)
        sData->listenFunction     = listenFunction;

    if(acceptFunction)
        sData->acceptFunction    = acceptFunction;

    if(connectFunction)
        sData->connectFunction     = connectFunction;

    if(sendFunction)
        sData->sendFunction     = sendFunction;
}

void SocketLayerStoreLocalData(
    Node *node,
    int fd,
    void *data)
{
    SLSocketData *sData = node->slData->socketList[fd];
    assert(sData);
    
    sData->localData = data;
}

void *SocketLayerGetLocalData(
    Node *node,
    int fd)
{
    SLSocketData *sData = node->slData->socketList[fd];
    assert(sData);
    
    return sData->localData;
}
