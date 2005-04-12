/*	$OpenBSD: dispatch.c,v 1.31 2004/09/21 04:07:03 david Exp $	*/

/*
 * Copyright 2004 Henning Brauer <henning@openbsd.org>
 * Copyright (c) 1995, 1996, 1997, 1998, 1999
 * The Internet Software Consortium.   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The Internet Software Consortium nor the names
 *    of its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INTERNET SOFTWARE CONSORTIUM AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE INTERNET SOFTWARE CONSORTIUM OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This software has been written for the Internet Software Consortium
 * by Ted Lemon <mellon@fugue.com> in cooperation with Vixie
 * Enterprises.  To learn more about the Internet Software Consortium,
 * see ``http://www.vix.com/isc''.  To learn more about Vixie
 * Enterprises, see ``http://www.vix.com''.
 */

#include "rosdhcp.h"
#include "dhcpd.h"
//#include <sys/ioctl.h>

//#include <net/if_media.h>
//#include <ifaddrs.h>
//#include <poll.h>

struct protocol *protocols = NULL;
struct timeout *timeouts = NULL;
static struct timeout *free_timeouts = NULL;
static int interfaces_invalidated = FALSE;
void (*bootp_packet_handler)(struct interface_info *,
                             struct dhcp_packet *, int, unsigned int,
                             struct iaddr, struct hardware *);

static int interface_status(struct interface_info *ifinfo);

/*
 * Use getifaddrs() to get a list of all the attached interfaces.  For
 * each interface that's of type INET and not the loopback interface,
 * register that interface with the network I/O software, figure out
 * what subnet it's on, and add it to the list of interfaces.
 */
#if 0
void
discover_interfaces(struct interface_info *iface)
{
    struct ifaddrs *ifap, *ifa;
    struct sockaddr_in foo;
    struct ifreq *tif;

    if (getifaddrs(&ifap) != 0)
        error("getifaddrs failed");

    for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next) {
        if ((ifa->ifa_flags & IFF_LOOPBACK) ||
            (ifa->ifa_flags & IFF_POINTOPOINT) ||
            (!(ifa->ifa_flags & IFF_UP)))
            continue;


        if (strcmp(iface->name, ifa->ifa_name))
            continue;

        /*
         * If we have the capability, extract link information
         * and record it in a linked list.
         */
        if (ifa->ifa_addr->sa_family == AF_LINK) {
            struct sockaddr_dl *foo =
                (struct sockaddr_dl *)ifa->ifa_addr;

            iface->index = foo->sdl_index;
            iface->hw_address.hlen = foo->sdl_alen;
            iface->hw_address.htype = HTYPE_ETHER; /* XXX */
            memcpy(iface->hw_address.haddr,
                   LLADDR(foo), foo->sdl_alen);
        } else if (ifa->ifa_addr->sa_family == AF_INET) {
            struct iaddr addr;

            memcpy(&foo, ifa->ifa_addr, sizeof(foo));
            if (foo.sin_addr.s_addr == htonl(INADDR_LOOPBACK))
                continue;
            if (!iface->ifp) {
                int len = IFNAMSIZ + ifa->ifa_addr->sa_len;
                if ((tif = malloc(len)) == NULL)
                    error("no space to remember ifp");
                strlcpy(tif->ifr_name, ifa->ifa_name, IFNAMSIZ);
                memcpy(&tif->ifr_addr, ifa->ifa_addr,
                       ifa->ifa_addr->sa_len);
                iface->ifp = tif;
                iface->primary_address = foo.sin_addr;
            }
            addr.len = 4;
            memcpy(addr.iabuf, &foo.sin_addr.s_addr, addr.len);
        }
    }

    if (!iface->ifp)
        error("%s: not found", iface->name);

    /* Register the interface... */
    if_register_receive(iface);
    if_register_send(iface);
    add_protocol(iface->name, iface->rfdesc, got_one, iface);
    freeifaddrs(ifap);
}
#else
void
discover_interfaces(struct interface_info *iface)
{
    NTSTATUS Status;
    ULONG dim;
    char TmpName [IFNAMSIZ];

    PIP_ADAPTER_INFO pAdapterInfo;
    PIP_ADAPTER_INFO pAdapter = NULL;

    pAdapterInfo = malloc(sizeof(IP_ADAPTER_INFO));
    dim = sizeof(IP_ADAPTER_INFO);

    if (GetAdaptersInfo( pAdapterInfo, &dim) != ERROR_SUCCESS) {
        free(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO *) malloc (dim);
    }

    if ((Status = GetAdaptersInfo( pAdapterInfo, &dim)) != NO_ERROR) {
        note("Error %x", Status);
        free (pAdapterInfo);
        return;
    }

    for (pAdapter = pAdapterInfo; pAdapter; pAdapter = pAdapter->Next) {
        /* we only do ethernet */
        note("found: %s %x\n", pAdapter->AdapterName, pAdapter->Address);
        if (pAdapter->Type != MIB_IF_TYPE_ETHERNET) {
            continue;
        }
        note ("found ethernet\n");

        iface->hw_address.hlen = pAdapter->AddressLength;
        iface->hw_address.htype = HTYPE_ETHER;
        memcpy (&iface->hw_address.haddr[0],
                pAdapter->Address, iface->hw_address.hlen);

        if (pAdapter->IpAddressList.IpAddress.String)
            iface->primary_address.S_un.S_addr = inet_addr(pAdapter->IpAddressList.IpAddress.String);

    }
    if (iface) {
        if_register_receive(iface);
        if_register_send(iface);
        add_protocol(iface->name, iface->rfdesc, got_one, iface);
    }
    free (pAdapterInfo);
}
#endif

void
reinitialize_interfaces(void)
{
    interfaces_invalidated = 1;
}

/*
 * Wait for packets to come in using poll().  When a packet comes in,
 * call receive_packet to receive the packet and possibly strip hardware
 * addressing information from it, and then call through the
 * bootp_packet_handler hook to try to do something with it.
 */
void
dispatch(void)
{
    int count, i, to_msec, nfds = 0;
    struct protocol *l;
    fd_set fds;
    time_t howlong;
    struct timeval timeval;

    ApiLock();

    for (l = protocols; l; l = l->next)
        nfds++;

    FD_ZERO(&fds);

//	fds = malloc(nfds * sizeof(struct pollfd));
//	if (fds == NULL)
//		error("Can't allocate poll structures.");

    do {
        DH_DbgPrint(MID_TRACE,("Cycling dispatch()\n"));
        /*
         * Call any expired timeouts, and then if there's still
         * a timeout registered, time out the select call then.
         */
    another:
        if (timeouts) {
            DH_DbgPrint(MID_TRACE,("Some timeouts are available\n"));
            
            struct timeout *t;

            if (timeouts->when <= cur_time) {
                DH_DbgPrint(MID_TRACE,("Calling timeout %x %p %x\n", 
                                       timeouts->when,
                                       timeouts->func,
                                       timeouts->what));
                t = timeouts;
                timeouts = timeouts->next;
                (*(t->func))(t->what);
                t->next = free_timeouts;
                free_timeouts = t;
                goto another;
            }

            /*
             * Figure timeout in milliseconds, and check for
             * potential overflow, so we can cram into an
             * int for poll, while not polling with a
             * negative timeout and blocking indefinitely.
             */
            howlong = timeouts->when - cur_time;
            if (howlong > INT_MAX / 1000)
                howlong = INT_MAX / 1000;
            to_msec = howlong * 1000;
        } else
            to_msec = -1;

        /* Set up the descriptors to be polled. */
        for (i = 0, l = protocols; l; l = l->next) {
            struct interface_info *ip = l->local;

            if (ip && (l->handler != got_one || !ip->dead)) {
                DH_DbgPrint(MID_TRACE,("l->fd %d\n", l->fd));
                        
                FD_SET(l->fd, &fds);
//				fds[i].fd = l->fd;
//				fds[i].events = POLLIN;
//				fds[i].revents = 0;
                i++;
            }
        }

        if (i == 0)
            error("No live interfaces to poll on - exiting.");

        /* Wait for a packet or a timeout... XXX */
        timeval.tv_sec = to_msec / 1000;
        timeval.tv_usec = (to_msec % 1000) * 1000;
        DH_DbgPrint(MID_TRACE,("select(%d,%d.%03d) =>\n", 
                 nfds,timeval.tv_sec,timeval.tv_usec/1000));

        ApiUnlock();

        count = select(nfds, &fds, NULL, NULL, &timeval);
        DH_DbgPrint(MID_TRACE,(" => %d\n", count));

        ApiLock();

        /* Not likely to be transitory... */
        if (count == SOCKET_ERROR) {
            if (errno == EAGAIN || errno == EINTR) {
                time(&cur_time);
                continue;
            } else
                error("poll: %m");
        }

        /* Get the current time... */
        time(&cur_time);
        
        i = 0;
        for (l = protocols; l; l = l->next) {
            struct interface_info *ip;
            ip = l->local;
            if (!FD_ISSET(l->fd, &fds)) {
//.revents & (POLLIN | POLLHUP))) {
//				fds[i].revents = 0;
                if (ip && (l->handler != got_one ||
                           !ip->dead)) {
                    DH_DbgPrint(MID_TRACE,("Handling %x\n", l));
                    (*(l->handler))(l);
                    if (interfaces_invalidated)
                        break;
                }
                i++;
            }
            interfaces_invalidated = 0;
        }
        DH_DbgPrint(MID_TRACE,("Done\n"));
    } while (1);

    ApiUnlock(); /* Not reached currently */
}

void
got_one(struct protocol *l)
{
    struct sockaddr_in from;
    struct hardware hfrom;
    struct iaddr ifrom;
    ssize_t result;
    union {
        /*
         * Packet input buffer.  Must be as large as largest
         * possible MTU.
         */
        unsigned char packbuf[4095];
        struct dhcp_packet packet;
    } u;
    struct interface_info *ip = l->local;

    if ((result = receive_packet(ip, u.packbuf, sizeof(u), &from,
                                 &hfrom)) == -1) {
        warning("receive_packet failed on %s: %s", ip->name,
                strerror(errno));
        ip->errors++;
        if ((!interface_status(ip)) ||
            (ip->noifmedia && ip->errors > 20)) {
            /* our interface has gone away. */
            warning("Interface %s no longer appears valid.",
                    ip->name);
            ip->dead = 1;
            interfaces_invalidated = 1;
            close(l->fd);
            remove_protocol(l);
            free(ip);
        }
        return;
    }
    if (result == 0)
        return;

    if (bootp_packet_handler) {
        ifrom.len = 4;
        memcpy(ifrom.iabuf, &from.sin_addr, ifrom.len);

        (*bootp_packet_handler)(ip, &u.packet, result,
                                from.sin_port, ifrom, &hfrom);
    }
}

#if 0
int
interface_status(struct interface_info *ifinfo)
{
    char *ifname = ifinfo->name;
    int ifsock = ifinfo->rfdesc;
    struct ifreq ifr;
    struct ifmediareq ifmr;

    /* get interface flags */
    memset(&ifr, 0, sizeof(ifr));
    strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
    if (ioctl(ifsock, SIOCGIFFLAGS, &ifr) < 0) {
        syslog(LOG_ERR, "ioctl(SIOCGIFFLAGS) on %s: %m", ifname);
        goto inactive;
    }

    /*
     * if one of UP and RUNNING flags is dropped,
     * the interface is not active.
     */
    if ((ifr.ifr_flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING))
        goto inactive;

    /* Next, check carrier on the interface, if possible */
    if (ifinfo->noifmedia)
        goto active;
    memset(&ifmr, 0, sizeof(ifmr));
    strlcpy(ifmr.ifm_name, ifname, sizeof(ifmr.ifm_name));
    if (ioctl(ifsock, SIOCGIFMEDIA, (caddr_t)&ifmr) < 0) {
        if (errno != EINVAL) {
            syslog(LOG_DEBUG, "ioctl(SIOCGIFMEDIA) on %s: %m",
                   ifname);

            ifinfo->noifmedia = 1;
            goto active;
        }
        /*
         * EINVAL (or ENOTTY) simply means that the interface
         * does not support the SIOCGIFMEDIA ioctl. We regard it alive.
         */
        ifinfo->noifmedia = 1;
        goto active;
    }
    if (ifmr.ifm_status & IFM_AVALID) {
        switch (ifmr.ifm_active & IFM_NMASK) {
        case IFM_ETHER:
            if (ifmr.ifm_status & IFM_ACTIVE)
                goto active;
            else
                goto inactive;
            break;
        default:
            goto inactive;
        }
    }
inactive:
    return (0);
active:
    return (1);
}
#else
int
interface_status(struct interface_info *ifinfo)
{
    return (1);
}
#endif

void
add_timeout(time_t when, void (*where)(void *), void *what)
{
    struct timeout *t, *q;

    DH_DbgPrint(MID_TRACE,("Adding timeout %x %p %x\n", when, where, what));
    /* See if this timeout supersedes an existing timeout. */
    t = NULL;
    for (q = timeouts; q; q = q->next) {
        if (q->func == where && q->what == what) {
            if (t)
                t->next = q->next;
            else
                timeouts = q->next;
            break;
        }
        t = q;
    }

    /* If we didn't supersede a timeout, allocate a timeout
       structure now. */
    if (!q) {
        if (free_timeouts) {
            q = free_timeouts;
            free_timeouts = q->next;
            q->func = where;
            q->what = what;
        } else {
            q = malloc(sizeof(struct timeout));
            if (!q)
                error("Can't allocate timeout structure!");
            q->func = where;
            q->what = what;
        }
    }

    q->when = when;

    /* Now sort this timeout into the timeout list. */

    /* Beginning of list? */
    if (!timeouts || timeouts->when > q->when) {
        q->next = timeouts;
        timeouts = q;
        return;
    }

    /* Middle of list? */
    for (t = timeouts; t->next; t = t->next) {
        if (t->next->when > q->when) {
            q->next = t->next;
            t->next = q;
            return;
        }
    }

    /* End of list. */
    t->next = q;
    q->next = NULL;
}

void
cancel_timeout(void (*where)(void *), void *what)
{
    struct timeout *t, *q;

    /* Look for this timeout on the list, and unlink it if we find it. */
    t = NULL;
    for (q = timeouts; q; q = q->next) {
        if (q->func == where && q->what == what) {
            if (t)
                t->next = q->next;
            else
                timeouts = q->next;
            break;
        }
        t = q;
    }

    /* If we found the timeout, put it on the free list. */
    if (q) {
        q->next = free_timeouts;
        free_timeouts = q;
    }
}

/* Add a protocol to the list of protocols... */
void
add_protocol(char *name, int fd, void (*handler)(struct protocol *),
             void *local)
{
    struct protocol *p;

    p = malloc(sizeof(*p));
    if (!p)
        error("can't allocate protocol struct for %s", name);

    p->fd = fd;
    p->handler = handler;
    p->local = local;
    p->next = protocols;
    protocols = p;
}

void
remove_protocol(struct protocol *proto)
{
    struct protocol *p, *next, *prev;

    prev = NULL;
    for (p = protocols; p; p = next) {
        next = p->next;
        if (p == proto) {
            if (prev)
                prev->next = p->next;
            else
                protocols = p->next;
            free(p);
        }
    }
}

int
interface_link_status(char *ifname)
{
#if 0
    struct ifmediareq ifmr;
    int sock;

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        error("Can't create socket");

    memset(&ifmr, 0, sizeof(ifmr));
    strlcpy(ifmr.ifm_name, ifname, sizeof(ifmr.ifm_name));
    if (ioctl(sock, SIOCGIFMEDIA, (caddr_t)&ifmr) == -1) {
        /* EINVAL -> link state unknown. treat as active */
        if (errno != EINVAL)
            syslog(LOG_DEBUG, "ioctl(SIOCGIFMEDIA) on %s: %m",
                   ifname);
        close(sock);
        return (1);
    }
    close(sock);

    if (ifmr.ifm_status & IFM_AVALID) {
        if ((ifmr.ifm_active & IFM_NMASK) == IFM_ETHER) {
            if (ifmr.ifm_status & IFM_ACTIVE)
                return (1);
            else
                return (0);
        }
    }
#endif
    return (1);
}
