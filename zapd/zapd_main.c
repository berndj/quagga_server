/* --------------------------------------------------------------
 * ZAPd main
 * zapd_main.c 
 */

#include <zebra.h>
#include "zclient.h"
#include "log.h"
#include "version.h"
#include <getopt.h>
#include "command.h"
#include "linklist.h"
#include "thread.h"
#include "prefix.h"
#include <signal.h>

#define ZAPD_DEFAULT_CONFIG "zapd.conf"
#define ZEBRA_VERSION "0.99.17"
#define ZAPD_VTY_PORT 26666
#define ZAPD_VTYSH_PATH "/tmp/.zapd"

/*
 * *  GLOBALS
 */

char config_current[] = ZAPD_DEFAULT_CONFIG;
char config_default[] = SYSCONFDIR ZAPD_DEFAULT_CONFIG;

struct option longopts[] = { { "daemon", no_argument, NULL, 'd' }, {
		"config_file", required_argument, NULL, 'f' }, { "help", no_argument,
		NULL, 'h' }, { "vty_port", required_argument, NULL, 'P' }, { "version",
		no_argument, NULL, 'v' }, { 0 } };

char* progname;
struct thread_master *master;

void signal_init(void);

/* These 2 are defined somewhere else, say in libzapd.a
 */
#ifdef REALLY_DUMMY
void zap_init(void) {return;};
void zap_terminate(void) {return;};
#else
void zap_init(void);
void zap_terminate(void);
#endif

/* ZAPD master for system wide configuration and variables. */
struct zapd_master {
	/* zapd instance. */
	struct list *zapd;

	/* zapd thread master. */
	struct thread_master *master;

	/* Zebra interface list. */
	struct list *iflist;

	/* Redistributed external information. */
	struct route_table *external_info[ZEBRA_ROUTE_MAX + 1];
#define EXTERNAL_INFO(T)      om->external_info[T]

	/* zapd start time. */
	time_t start_time;

	/* Various zapd global configuration. */
	u_char options; 
#define OSPF_MASTER_SHUTDOWN (1 << 0) /* deferred-shutdown */  
};

static struct zapd_master zapd_master;
/* ZAPD process wide configuration pointer to export. */ 
struct zapd_master *om;


void zapd_master_init() {
	memset(&zapd_master, 0, sizeof(struct zapd_master));

	om = &zapd_master;
	om->zapd = list_new();
	om->master = thread_master_create();
	om->start_time = quagga_time(NULL);
}

/* Help information display. */
static void usage(int status) {
	if (status != 0)
		fprintf(stderr, "Try `%s --help' for more information.\n", progname);
	else {
		printf(
				"Usage : %s [OPTION...]\n\
Daemon which manages ZAP.\n\n\
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

extern struct zclient* zclient = NULL;
/* might as well be defined here and exported over zap_zebra.h
 * as it could be needed somewhere else
 */

/* Inteface addition message from zebra. */ 
int zap_interface_add(int command, struct zclient *zclient, zebra_size_t length) {
	struct interface *ifp;
	ifp = zebra_interface_add_read(zclient->ibuf);
	zlog_info ("Zebra: interface add %s index %d flags %d metric %d mtu %d  hw addr len %d",
	       ifp->name, ifp->ifindex, ifp->flags, ifp->metric, ifp->mtu, ifp->hw_addr_len);

	return 0;
}


void zclient_read_zapi_ipv4( struct zclient* zclient,
 struct zapi_ipv4 *zapi, struct prefix_ipv4* p,
 unsigned long* ifindex,  struct in_addr* nexthop) 
{
  struct stream *s;
  s = zclient->ibuf;

/* read the header */
  zapi->type = stream_getc (s);
  zapi->flags = stream_getc (s);
  zapi->message = stream_getc (s);

/* and the prefix */
  memset (p, 0, sizeof (struct prefix_ipv4));
  p->family = AF_INET;
  p->prefixlen = stream_getc (s);
  stream_get (&p->prefix, s, PSIZE (p->prefixlen));

  if (CHECK_FLAG (zapi->message, ZAPI_MESSAGE_NEXTHOP))
    {
      zapi->nexthop_num = stream_getc (s);
      nexthop->s_addr = stream_get_ipv4 (s);
    }
  if (CHECK_FLAG (zapi->message, ZAPI_MESSAGE_IFINDEX))
    {
      zapi->ifindex_num = stream_getc (s);
      *ifindex = stream_getl (s);
    }
  if (CHECK_FLAG (zapi->message, ZAPI_MESSAGE_DISTANCE))
    zapi->distance = stream_getc (s);
  if (CHECK_FLAG (zapi->message, ZAPI_MESSAGE_METRIC))
    zapi->metric = stream_getl (s);

}
 
int
zap_interface_address_add (int command, struct zclient *zclient,
                            zebra_size_t length)
{
  struct connected *c;
  struct prefix *p;

/* read the address from the zebra protocol stream */
  c = zebra_interface_address_read(ZEBRA_INTERFACE_ADDRESS_ADD, zclient->ibuf);  // ZEBRA_INTERFACE_ADDRESS_ADD
  if (c == NULL)
    return 0;
  p = c->address;
  if (p->family == AF_INET)
   zlog_info (" new connected IPv4 address %s/%d on interface %s", 
                inet_ntoa (p->u.prefix4), p->prefixlen, c->ifp->name);
  else if(p->family == AF_INET6)
   zlog_info (" new connected IPv6 address on interface %s", c->ifp->name);
  return 0;
}




void
zapd_zebra_add (struct prefix_ipv4 *p, struct ospf_route *or)
{
  u_char message;
  u_char distance;
  u_char flags;
  int psize;
  struct stream *s;
  struct ospf_path *path;
  struct listnode *node;

printf(" Reached zapd_zebra_add ");
 // if (zclient->redist[ZEBRA_ROUTE_ZAP])
   // {
      message = 0;
      flags = 0;

zlog_info (" Adding Routes ");
      /* OSPF pass nexthop and metric */
      SET_FLAG (message, ZAPI_MESSAGE_NEXTHOP);
      SET_FLAG (message, ZAPI_MESSAGE_METRIC);
      /* Make packet. */
      s = zclient->obuf; 
      stream_reset (s);

      /* Put command, type, flags, message. */
      zclient_create_header (s, ZEBRA_IPV4_ROUTE_ADD);
      stream_putc (s, ZEBRA_ROUTE_ZAP);
      stream_putc (s, flags);
      stream_putc (s, message);

      /* Put prefix information. */
      psize = PSIZE (p->prefixlen);
      stream_putc (s, p->prefixlen);
      stream_write (s, (u_char *) & p->prefix, psize);

      stream_putw_at (s, 0, stream_get_endp (s));

      zclient_send_message(zclient);
   // }
}






void zap_zebra_init(void) {
	int i;

	zclient = zclient_new();
	zclient_init(zclient, ZEBRA_ROUTE_ZAP);

	 for(i=0; i++ < ZEBRA_ROUTE_MAX;) 
	 if( i != ZEBRA_ROUTE_ZAP) 
	 zclient_redistribute(ZEBRA_REDISTRIBUTE_ADD, zclient, i);


   zclient->interface_add = zap_interface_add;
   // zclient->ipv4_route_add = zap_zebra_route_manage;
   zclient->interface_address_add = zap_interface_address_add;

}


void zap_add_route() {

	struct route_node *rn;
 	struct ospf_route *or;
 	/* IPv4 prefix. */
 	struct prefix_ipv4 p;
    memset (&p, 0, sizeof (struct prefix_ipv4));
    p.family = AF_INET;
    p.prefix.s_addr = 0xa000000;
    p.prefixlen = 24;
	
	printf(" ++++++++++ ZAP add route +++++++++++ ");
	zapd_zebra_add((struct prefix_ipv4 *) &p, or);
	
	
	  /* Install new routes. 
  for (rn = route_top (rt); rn; rn = route_next (rn))
    if ((or = rn->info) != NULL)
      {
	if (or->type == OSPF_DESTINATION_NETWORK)
	  {
	    if (! ospf_route_match_same (ospf->old_table,
					 (struct prefix_ipv4 *)&rn->p, or))
	      ospf_zebra_add ((struct prefix_ipv4 *) &rn->p, or);
	  }
	  }
*/
}

int main(int argc, char** argv, char** envp) {
	char *p;
	char *vty_addr = "localhost";
	int vty_port = 0;
	int daemon_mode = 0;
	char *config_file = NULL;
	struct thread thread;

	umask(0027);

	progname = ((p = strrchr(argv[0], '/')) ? ++p : argv[0]);
	/*
	 zlog_default = openzlog (progname, ZLOG_NOLOG, ZLOG_ZAP, 
	 LOG_CONS|LOG_NDELAY|LOG_PID, LOG_DAEMON);  */
	zlog_default = openzlog(progname, ZLOG_ZAPD, LOG_CONS | LOG_NDELAY
			| LOG_PID, LOG_DAEMON);

	/* initialize the log subsystem, you will have to include
	 * ZLOG_ZAP in the zlog_proto_t enum type definition in 
	 * lib/log.h
	 */

	/* this while just reads the options */
	while (1) {
		int opt;

		opt = getopt_long(argc, argv, "dlf:hP:v", longopts, 0);

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
			//print_version ();
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

	/* one to control them all, ... */
	master = thread_master_create();
	/* this the main thread controlling structure,
	 * nothing to remember.
	 */

	signal_init();
	/* before you start the engine, put your safety belt on
	 */

	/* 
	 * * Library inits.
	 */
	cmd_init(1);
	/* initializes the command sub-system, if arg, add some commands
	 * which are mostly only useful for humans on the vty
	 */

	vty_init(master);
	memory_init();
    if_init();
	access_list_init();
	prefix_list_init();
	/* these are all from libzebra
	 */

	zapd_master_init();
	zap_zebra_init();
	zap_add_route();

	/*
	 * ZAP inits
	 */
	// zap_init();
	/* this is implemented somewhere, e.g. on libzap.a
	 * here, you could start some threads (the thread subsystem
	 * is not running yet), register some commands, ...
	 */

	sort_node();
	/* This is needed by the command subsystem to finish initialization.
	 */

	/* Get configuration file. */
	vty_read_config(config_file, config_default);
	/* read the config file, your commands should be defined before this
	 */

	/* Change to the daemon program. */
	if (daemon_mode)
		daemon(0, 0);

	/* Create VTY socket */
	//   vty_serv_sock (vty_port ? vty_port : ZAPD_VTY_PORT, ZAPD_VTYSH_PATH);
	vty_serv_sock(vty_addr, ZAPD_VTY_PORT, ZAPD_VTYSH_PATH);
	/* start the TCP and unix socket listeners
	 */

	/* Print banner. */
	zlog(NULL, LOG_INFO, "ZAP (%s) starts", ZEBRA_VERSION);

	/* Fetch next active thread. */
	while (thread_fetch(master, &thread))
		thread_call(&thread);
	/* this is the main event loop */

	/* never reached */
	return 0;
}

/* SIGHUP handler. */
void sighup(int sig) {
	zlog(NULL, LOG_INFO, "SIGHUP received");
}

/* SIGINT handler. */
void sigint(int sig) {
	zlog(NULL, LOG_INFO, "Terminating on signal");

	// zap_terminate ();
	/* this is your clean-up function */

	exit(0);
}

/* SIGUSR1 handler. */
void sigusr1(int sig) {
	zlog_rotate(NULL);
}

#define RETSIGTYPE void
/* Signal wrapper. */
RETSIGTYPE *
signal_set(int signo, void(*func)(int)) {
	int ret;
	struct sigaction sig;
	struct sigaction osig;

	sig.sa_handler = func;
	sigemptyset(&sig.sa_mask);
	sig.sa_flags = 0;
#ifdef SA_RESTART
	sig.sa_flags |= SA_RESTART;
#endif /* SA_RESTART */

	ret = sigaction(signo, &sig, &osig);

	if (ret < 0)
		return (SIG_ERR);
	else
		return (osig.sa_handler);
}

/* Initialization of signal handles. */
void signal_init() {
	signal_set(SIGHUP, sighup);
	signal_set(SIGINT, sigint);
	signal_set(SIGTERM, sigint);
	signal_set(SIGPIPE, SIG_IGN);
#ifdef SIGTSTP
	signal_set(SIGTSTP, SIG_IGN);
#endif
#ifdef SIGTTIN
	signal_set(SIGTTIN, SIG_IGN);
#endif
#ifdef SIGTTOU
	signal_set(SIGTTOU, SIG_IGN);
#endif
	signal_set(SIGUSR1, sigusr1);
}

/* zapd_main.c ends 
 ----------------------------------------------------------------
 */

