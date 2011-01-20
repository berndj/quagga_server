/*
 * main_daemon.c
 *
 *  Created on: Jan 7, 2011
 *      Author: smartin
 *      gcc -o main_d -lm -lcrypt -I. -I.. -I../lib main_daemon.c ../lib/libzebra.a
 *      gcc -o main_d -lm -lcrypt -I. -I.. -I../lib main_daemon.c ../lib/.libs/libzebra.a
 *      gcc -Wall -o new_daemon -lm -lcrypt -lprotoc-c -I. -I.. -I../lib daemon_test.c test.pb-c.c ../lib/.libs/libzebra.a
 *
 *
 /.configure --enable-isisd --enable-nssa --enable-ospf-te --enable-opaque-lsa --enable-multipath=6
 --enable-vtysh --enable-tcp-zebra --enable-snmp	--with-libpam --enable-netlink
 *
 *
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <zebra.h>
#include "version.h"
#include "getopt.h"
#include "thread.h"
#include "command.h"
#include "prefix.h"
#include "log.h"
#include "zclient.h"
#include "route.pb-c.h"

#define MY_DEFAULT_CONFIG "zapd.conf"
#define MY_VTY_PORT 26660
#define MY_VTYSH_PATH "/tmp/.zapd"
#define SYSCONFDIR "/usr/local/etc/"
#define DEBUG_STR 1

/* All information about zebra. */
struct zclient *zclient = NULL;

/* daemon options. */
static struct option
		longopts[] = { { "daemon", no_argument, NULL, 'd' }, { "config_file",
				required_argument, NULL, 'f' }, { "help", no_argument, NULL,
				'h' }, { "vty_port", required_argument, NULL, 'P' }, {
				"retain", no_argument, NULL, 'r' }, { "version", no_argument,
				NULL, 'v' }, { 0 } };

/* Configuration file and directory. */
char config_current[] = MY_DEFAULT_CONFIG;
char config_default[] = SYSCONFDIR MY_DEFAULT_CONFIG;
char *config_file = NULL;

/* program name */
char *progname;

/* RIP VTY connection port. */
int vty_port = MY_VTY_PORT;

/* Master of threads. */
struct thread_master *master;

/* Help information display. */
static void usage(int status) {
	if (status != 0)
		fprintf(stderr, "Try `%s --help' for more information.\n", progname);
	else {
		printf(
				"Usage : %s [OPTION...]\n\
A test Daemon for adding and deleting routes.\n\n\
-d, --daemon       Runs in daemon mode\n\
-f, --config_file  Set configuration file name\n\
-P, --vty_port     Set vty's port number\n\
-v, --version      Print program version\n\
-h, --help         Display this help and exit\n\
\n\
Report bugs to %s\n",
				progname, ZEBRA_BUG_ADDRESS);
	}

	exit(status);
}
void error(char *msg) {
	perror(msg);
	exit(1);
}

int my_ip_addr_delete(struct zclient *zclient) {
	my_ip_addr_modify(zclient, ZEBRA_IPV4_ROUTE_DELETE);
}

int my_ip_addr_add(struct zclient *zclient) {
	my_ip_addr_modify(zclient, ZEBRA_IPV4_ROUTE_ADD);
}
int my_ip_addr_modify(struct zclient *zclient, int operation) {

	struct zapi_ipv4 zr;
	struct prefix_ipv4 p;
	struct in_addr newly_added_address;
	struct in_addr newly_added_nexthop_address;
	struct in_addr *ptr_newly_added_nexthop_address;
	unsigned int ifindex_value;
	int ret;

	memset((void*) &zr, 0, sizeof(zr));
	memset((void*) &p, 0, sizeof(p));

	p.family = AF_INET;
	p.prefixlen = 24;
	inet_pton(AF_INET, "152.24.120.0", &newly_added_address);
	inet_pton(AF_INET, "10.39.4.1", &newly_added_nexthop_address);

	p.prefix = newly_added_address;
	zr.type = ZEBRA_ROUTE_STATIC;
	zr.flags = 0;
	SET_FLAG(zr.message, ZAPI_MESSAGE_NEXTHOP);
	// SET_FLAG (zr.message, ZAPI_MESSAGE_METRIC);

	zr.nexthop_num = 1;
	ptr_newly_added_nexthop_address = &newly_added_nexthop_address;
	zr.nexthop = &ptr_newly_added_nexthop_address;

	//	zr.metric = 111;
	//	zr.distance = 0;
	//	zr.ifindex_num = 1;
	//	ifindex_value = 1;
	//	zr.ifindex = &ifindex_value;

	ret = zapi_ipv4_route(operation, zclient, &p, &zr);
	printf("Return val from %s is: %d \n",
			operation == ZEBRA_IPV4_ROUTE_ADD ? "ZEBRA_IPV4_ROUTE_ADD"
					: "ZEBRA_IPV4_ROUTE_DELETE", ret);
	printf("Return val %d \n", ret);
	return 0;
}

#if 0
int
my_ip_addr_delete(struct zclient *zclient) {

	struct zapi_ipv4 zr;
	struct prefix_ipv4 *p;
	struct prefix_ipv4 p2;
	struct in_addr newly_added_address;
	struct in_addr newly_added_nexthop_address;
	struct in_addr *ptr_newly_added_nexthop_address;
	unsigned int ifindex_value;
	int ret;

	p = &p2;

	p->family = AF_INET;
	p->prefixlen = 24;
	inet_pton(AF_INET, "172.24.120.0", &newly_added_address);
	inet_pton(AF_INET, "10.76.105.254", &newly_added_nexthop_address);

	p->prefix = newly_added_address;
	zr.type = ZEBRA_ROUTE_MY;
	zr.flags = 0;
	zr.message = 0;
	SET_FLAG (zr.message, ZAPI_MESSAGE_NEXTHOP);
	SET_FLAG (zr.message, ZAPI_MESSAGE_METRIC);
	zr.nexthop_num = 0;
	ptr_newly_added_nexthop_address = &newly_added_nexthop_address;
	zr.nexthop = &ptr_newly_added_nexthop_address;
	zr.metric = 111;
	zr.distance = 0;
	zr.ifindex_num = 1;
	ifindex_value = 1;
	zr.ifindex = &ifindex_value;

	//    zr.type = ZEBRA_ROUTE_MY;
	//    zr.flags = 0;
	//    zr.message = 0;
	//    p->family = AF_INET;
	//    p->prefixlen = '8';
	//    p->prefix = newly_added_address;
	ret = zapi_ipv4_route (ZEBRA_IPV4_ROUTE_DELETE, zclient, p, &zr);
	// ret = zebra_message_send(zclient, 8);
	printf("Return val from ZEBRA_IPV4_ROUTE_DELETE is: %d \n", ret);
	return 0;
}
#endif

/* Interface addition message from zebra. */
int zap_interface_add(struct zclient *zclient) {
	struct interface *ifp;

	ifp = zebra_interface_add_read(zclient->ibuf);

	zlog_info(
			"Zebra: interface add %s index %d flags %X metric %d mtu %d  hw addr len %d",
			ifp->name, ifp->ifindex, ifp->flags, ifp->metric, ifp->mtu,
			ifp->hw_addr_len);

	return 0;
}

int run_sock_client(int port_no) {
	int sockfd, portno, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	char buffer[256];

	Route route = ROUTE__INIT; // Route
	void *buf; // Buffer to store serialized data
	unsigned len; // Length of serialized data

	route.destination_ip = "10.36.40.1";
	route.subnet_mask = "255.0.0.0";
	route.nexthop_ip = "10.50.60.3";
	route.metric = 400;

	len = route__get_packed_size(&route); // This is calculated packing length

	buf = malloc(len); // Allocated memory for packed/encode serialized data
	route__pack(&route, buf); // Put it in buf now

	portno = port_no;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		error("ERROR opening socket");
	server = gethostbyname("localhost");
	if (server == NULL) {
		fprintf(stderr, "ERROR, no such host\n");
		exit(0);
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr,
			server->h_length);
	serv_addr.sin_port = htons(portno);
	if (connect(sockfd, &serv_addr, sizeof(serv_addr)) < 0)
		error("ERROR connecting");

	n = write(sockfd, buf, len);
	if (n < 0)
		error("ERROR writing to socket");
	n = read(sockfd, buffer, 255);
	if (n < 0)
		error("ERROR reading from socket");
	printf("Received back: %s \n", buffer);
	free(buf);
	return 0;
}

void my_init() {
	int i;
	int ret_client_start;
	struct interface *ifp;

	/* Set default value to the zebra client structure. */
	zclient = zclient_new();
	zclient_init(zclient, ZEBRA_ROUTE_MY);
	ret_client_start = zclient_start(zclient);

	for (i = 0; i++ < ZEBRA_ROUTE_MAX;)
		if (i != ZEBRA_ROUTE_MY)
			zebra_redistribute_send(ZEBRA_REDISTRIBUTE_ADD, zclient, i);

	/* Call the functions for adding and deleting routes */
	//my_ip_addr_add(zclient);
	my_ip_addr_delete(zclient);

	// zclient->ipv4_route_add = my_ip_addr_add;
	// zclient->ipv4_route_delete = my_ip_addr_delete;
}

/* Main routine of our daemom */
int main(int argc, char **argv) {
	char *p;
	char *vty_addr = "localhost";
	int daemon_mode = 0;
	struct thread thread;

	/* Set umask before anything for security */
	umask(0027);

	/* Get program name. */
	progname = ((p = strrchr(argv[0], '/')) ? ++p : argv[0]);

	/* First of all we need logging init. */
	zlog_default = openzlog(progname, ZLOG_ZAPD, LOG_CONS | LOG_NDELAY
			| LOG_PID, LOG_DAEMON);

	/* Command line option parse. */
	while (1) {
		int opt;

		opt = getopt_long(argc, argv, "df:hP:v", longopts, 0);

		if (opt == EOF)
			break;

		switch (opt) {
		case 0:
			break;
		case 'd':
			daemon_mode = 1;
			break;
		case 'f':
			config_file = optarg;
			break;
		case 'P':
			vty_port = atoi(optarg);
			break;
		case 'v':
			print_version(progname);
			exit(0);
			break;
		case 'h':
			usage(0);
			break;
		default:
			usage(1);
			break;
		}
	}

	/* Prepare master thread. */
	master = thread_master_create();

	/* Library initialization. */
	cmd_init(1);
	vty_init(master);
	memory_init();
	keychain_init();

	my_init();
	/* Calling the function which will create a socket for
	   the protobuf communication. */
	// run_sock_client(26665);

	/* Sort all installed commands. */
	sort_node();

	/* Get configuration file. */
	vty_read_config(config_file, config_default);

	/* Change to the daemon program. */
	if (daemon_mode)
		daemon(0, 0);

	/* Pid file create. */
	// pid_output (PATH_RIPD_PID);

	/* Create VTY's socket */
	vty_serv_sock(vty_addr, MY_VTY_PORT, MY_VTYSH_PATH);

	/* Execute each thread. */
	while (thread_fetch(master, &thread))
		thread_call(&thread);

	/* Not reached. */
	exit(0);
}
