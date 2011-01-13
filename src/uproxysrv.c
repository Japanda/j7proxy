/*
 * UProxy - A small proxy program for UDP based protocols
 *
 * Copyright (C) 2000  Alessandro Staltari
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */


#ifdef WIN32
    #include <windows.h>
    #include <winsock.h>
#else
    #include <sys/ioctl.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <sys/time.h>
    #include <sys/types.h>
    #include <unistd.h>
    #include <stdlib.h>
#endif

#include <stdio.h>
#include <time.h>
#include <string.h>

#ifndef WIN32
    #define SOCKET      int
    #define closesocket close
    #define ioctlsocket ioctl
#endif

#define VERSION "0.91"

#define ARGNUM 3

SOCKET ProxySocket;
struct sockaddr_in ProxySAddr, ClientSAddr, TargetSAddr;
struct sockaddr AnySAddr={AF_INET, {INADDR_ANY}};
struct ClientList
{
    SOCKET sock;
    struct sockaddr_in saddr;
    time_t last;
    struct ClientList *next;
};
struct ClientList *Clients=NULL;

/* Winsock initialization code stolen from netcat :-) */
#ifdef WIN32

/* res_init
   winsock needs to be initialized. Might as well do it as the res_init
   call for Win32 */

void res_init()
{
    WORD wVersionRequested; 
    WSADATA wsaData; 
    int err; 
    wVersionRequested = MAKEWORD(1, 1); 
 
    err = WSAStartup(wVersionRequested, &wsaData); 
 
    if (err != 0) 
        /* Tell the user that we couldn't find a useable */ 
        /* winsock.dll.     */ 
        return; 
 
    /* Confirm that the Windows Sockets DLL supports 1.1.*/ 
    /* Note that if the DLL supports versions greater */ 
    /* than 1.1 in addition to 1.1, it will still return */ 
    /* 1.1 in wVersion since that is the version we */ 
    /* requested. */ 
 
    if ( LOBYTE( wsaData.wVersion ) != 1 || 
        HIBYTE( wsaData.wVersion ) != 1 ) { 
        /* Tell the user that we couldn't find a useable */ 
        /* winsock.dll. */ 
        WSACleanup(); 
        return; 
    }
 
}

#endif

int main(int argc, char *argv[])
{
    char recbuf[2048];
    int recbuflen;
    struct sockaddr_in fromaddr;
    unsigned int fromaddrlen;
    
    int bytes_in=0, bytes_out=0, old_bytes_in=0, old_bytes_out=0;
    time_t last_stat;
    unsigned long nonzero=1;

    printf("UProxy %s - Copyright (C) 2000 Alessandro Staltari", VERSION);
    printf("\nDistributed under the terms of the GNU General Public License 2.0\n\n");
    if (argc!=ARGNUM+1)
    {
        printf("uproxy LocalPort ServerAddress ServerPort\n");
        exit(EXIT_FAILURE);
    }
#ifdef WIN32
    res_init();
    SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS );
#endif

    last_stat=time(NULL);
    ProxySocket=socket(AF_INET, SOCK_DGRAM, 0);
    ioctlsocket ( ProxySocket, FIONBIO, &nonzero);
    memset(&ProxySAddr,0,sizeof(struct sockaddr_in));
    ProxySAddr.sin_family=AF_INET;
    ProxySAddr.sin_port=htons((unsigned short) atoi(argv[1]));
    ProxySAddr.sin_addr.s_addr=INADDR_ANY;
    bind(ProxySocket, (const struct sockaddr *) &ProxySAddr, sizeof(struct sockaddr_in));

    TargetSAddr.sin_family=AF_INET;
    TargetSAddr.sin_port=htons((unsigned short) atoi(argv[3]));
    TargetSAddr.sin_addr.s_addr=inet_addr(argv[2]);

    printf("\nProxy server started on port %d. Forwarding data to %s:%d\n", atoi(argv[1]), argv[2], atoi(argv[3]));
    while (1)
    {
#ifdef WIN32          
        Sleep(1);
#else
        sleep(1);
#endif    
    
        fd_set r_fdset;
        struct timeval tv;
        struct ClientList *client=Clients;
        int maxfd;
        time_t current_time;

        tv.tv_sec=1;
        tv.tv_usec=0;
        FD_ZERO(&r_fdset);
        FD_SET(ProxySocket, &r_fdset);
        maxfd=ProxySocket;
        if (Clients!=NULL)
        {
            struct ClientList *c=Clients;

            while (c!=NULL)
            {
                FD_SET(c->sock, &r_fdset);
                if (c->sock > maxfd) maxfd=c->sock;
                c=c->next;
            }
        }
        select(maxfd+1, &r_fdset, NULL, NULL, &tv);
        current_time=time(NULL);
        if ((current_time-last_stat)>=10)
        {
            printf("in %8d out %8d\n", bytes_in-old_bytes_in, bytes_out-old_bytes_out);
            old_bytes_out=bytes_out;
            old_bytes_in=bytes_in;
            last_stat=current_time;
        }
        if ((recbuflen=recvfrom(ProxySocket, recbuf, 2048, 0, (struct sockaddr *) &fromaddr, &fromaddrlen))>=0)
        {
            struct ClientList *c=Clients;
            
            while (c!=NULL)
            {
                if ((c->saddr.sin_addr.s_addr!=fromaddr.sin_addr.s_addr)
                    || (c->saddr.sin_port!=fromaddr.sin_port))
                c=c->next;
                else break;
            }
            if (c==NULL)
            {
                c=malloc(sizeof(struct ClientList));
                c->next=Clients;
                Clients=c;
                memcpy(&(c->saddr), &fromaddr, sizeof(struct sockaddr_in));
                printf("%s:%d Connected.\n", inet_ntoa(c->saddr.sin_addr), ntohs(c->saddr.sin_port));
                c->sock=socket(AF_INET, SOCK_DGRAM, 0);
                ioctlsocket (c->sock, FIONBIO, &nonzero);
                bind(c->sock, (const struct sockaddr *) &AnySAddr, sizeof(struct sockaddr_in));
            }
            //printf("OUT: %s:%d\n", inet_ntoa(fromaddr.sin_addr), ntohs(fromaddr.sin_port));
            sendto(c->sock, recbuf, recbuflen, 0, (const struct sockaddr *) &TargetSAddr,
                   sizeof(struct sockaddr_in));
            c->last=current_time;
            bytes_out+=recbuflen;
        }
        client=Clients;
        while(client!=NULL)
        {
            if ((recbuflen=recvfrom(client->sock, recbuf, 2048, 0, (struct sockaddr *) &fromaddr, &fromaddrlen))>=0)
            {
                //printf("IN: %s:%d\n", inet_ntoa(fromaddr.sin_addr), ntohs(fromaddr.sin_port));
                sendto(ProxySocket, recbuf, recbuflen, 0, (const struct sockaddr *) &(client->saddr),
                       sizeof(struct sockaddr_in));
                bytes_in+=recbuflen;
            }
            else if ((current_time-client->last)>30)
            {
                struct ClientList *c=Clients;

                closesocket(client->sock);
                printf("%s:%d Disconnected.\n", inet_ntoa(client->saddr.sin_addr), ntohs(client->saddr.sin_port));
                if (client!=Clients)
                {
                    while (c->next!=client)
                        c=c->next;
                    c->next=c->next->next;
                    free(client);
                    client=c;
                }
                else
                {
                    Clients=client->next;
                    free(client);
                    client=Clients;
                    continue;
                }
            }
            client=client->next;
        }
    }
}
