/**
 * @file kernel_socket.c
 *
 * @brief Implements socket interface function for kernel mode
 *
 * @details
 * @date 13.01.2010
 *
 * @author Anton Bondarev
 */
#include <common.h>
#include <kernel/irq.h>
#include <lib/list.h>
#include <net/socket.h>
#include <net/net.h>

typedef struct socket_info {
	/*it must be first member! We use casting in sock_realize function*/
	struct socket sock;
	struct list_head list __attribute__ ((aligned (4)));
} socket_info_t __attribute__ ((aligned (4)));

static socket_info_t sockets_pull[MAX_KERNEL_SOCKETS];
static LIST_HEAD(head_free_sk);

static const struct net_proto_family *net_families[NPROTO];

/* sock_alloc	-	allocate a socket
 * now we only allocate memory for structure of socket (struct socket)
 *
 * In Linux it must use socketfs and they use inodes for it
 */
static struct socket *sock_alloc(void) {
	struct socket *sock;
	struct list_head *entry;

	unsigned long irq_old = local_irq_save();

	if (list_empty(&head_free_sk)) {
		local_irq_restore(irq_old);
		return NULL;
	}
	entry = (&head_free_sk)->next;
	list_del_init(entry);
	sock = (struct socket *) list_entry(entry, socket_info_t, list);

	local_irq_restore(irq_old);

	return sock;
}

void kernel_sock_release(struct socket *sock) {
	socket_info_t *sock_info;
	unsigned long irq_old;
	if ((NULL == sock) || (NULL == sock->ops) || (NULL == sock->ops->release)) {
		return;
	}
	/*release struct sock*/
	sock->ops->release(sock);

	irq_old = local_irq_save();
	/*return sock into pull*/
	/* we can cast like this because struct socket is first element of
	 * struct socket_info
	 */
	sock_info = (socket_info_t *) sock;
	list_add(&sock_info->list, &head_free_sk);
	local_irq_restore(irq_old);
}

static int __sock_create(int family, int type, int protocol,
		struct socket **res, int kern) {
	int err;
	struct socket *sock;
	const struct net_proto_family *pf;

	/*
	 *      Check protocol is in range
	 */
	if (family < 0 || family >= NPROTO)
		return -1;
	if (type < 0 || type >= SOCK_MAX)
		return -1;

	if (family == PF_INET && type == SOCK_PACKET) {
		family = PF_PACKET;
	}
	/*pf = rcu_dereference(net_families[family]);*/
	pf = (const struct net_proto_family *) &net_families[family];
	if (NULL == pf || NULL == pf->create) {
		return -1;
	}
	/*
	 here must be code for trying socket (permition and so on)
	 err = security_socket_create(family, type, protocol, kern);
	 if (err)
	 return err;
	 */
	/*
	 *	Allocate the socket and allow the family to set things up. if
	 *	the protocol is 0, the family is instructed to select an appropriate
	 *	default.
	 */
	sock = sock_alloc();
	if (!sock) {
		return -1;
	}

	sock->type = type;

	err = pf->create(sock, protocol);
	if (err < 0) {
		kernel_sock_release(sock);
		return -1;
	}
	/*here we must be code for trying socket (permition and so on)
	 err = security_socket_post_create(sock, family, type, protocol, kern);
	 */
	*res = sock;
	return 0;
}

int kernel_sock_init(void) {
	int i;
	for (i = 0; i < array_len(sockets_pull); i++) {
		list_add(&(&sockets_pull[i])->list, &head_free_sk);
	}
	return 0;
}

int sock_create_kern(int family, int type, int protocol, struct socket **res) {
	return __sock_create(family, type, protocol, res, 1);
}

int kernel_bind(struct socket *sock, struct sockaddr *addr, int addrlen) {
	return sock->ops->bind(sock, addr, addrlen);
}

int kernel_listen(struct socket *sock, int backlog) {
	return sock->ops->listen(sock, backlog);
}

int kernel_accept(struct socket *sock, struct socket **newsock, int flags) {
	return sock->ops->accept(sock, *newsock, flags);
}

int kernel_connect(struct socket *sock, struct sockaddr *addr, int addrlen,
		int flags) {
	return sock->ops->connect(sock, addr, addrlen, flags);
}

int kernel_getsockname(struct socket *sock, struct sockaddr *addr, int *addrlen) {
	return sock->ops->getname(sock, addr, addrlen, 0);
}

int kernel_getpeername(struct socket *sock, struct sockaddr *addr, int *addrlen) {
	return sock->ops->getname(sock, addr, addrlen, 1);
}

int kernel_getsockopt(struct socket *sock, int level, int optname,
		char *optval, int *optlen) {
	return sock->ops->getsockopt(sock, level, optname, optval, optlen);
}

int kernel_setsockopt(struct socket *sock, int level, int optname,
		char *optval, int optlen) {
	return sock->ops->setsockopt(sock, level, optname, optval, optlen);
}

#if 0
int kernel_sendpage(struct socket *sock, struct page *page, int offset,
		size_t size, int flags) {
	if (sock->ops->sendpage)
		return sock->ops->sendpage(sock, page, offset, size, flags);

	return sock_no_sendpage(sock, page, offset, size, flags);
}

int kernel_sock_ioctl(struct socket *sock, int cmd, unsigned long arg) {
	mm_segment_t oldfs = get_fs();
	int err;

	set_fs(KERNEL_DS);
	err = sock->ops->ioctl(sock, cmd, arg);
	set_fs(oldfs);

	return err;
}
#endif
