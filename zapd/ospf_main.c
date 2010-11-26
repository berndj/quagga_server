
#include <zebra.h>

#include <lib/version.h>
#include "zclient.h"
#include "getopt.h"
#include "thread.h"
#include "prefix.h"
#include "linklist.h"
#include "if.h"
#include "vector.h"
#include "vty.h"
#include "command.h"
#include "filter.h"
#include "plist.h"
#include "stream.h"
#include "log.h"
#include "memory.h"
#include "privs.h"
#include "sigevent.h"

#include "ospfd/ospfd.h"
#include "ospfd/ospf_interface.h"
#include "ospfd/ospf_asbr.h"
#include "ospfd/ospf_lsa.h"
#include "ospfd/ospf_lsdb.h"
#include "ospfd/ospf_neighbor.h"
#include "ospfd/ospf_dump.h"
#include "ospfd/ospf_zebra.h"
#include "ospfd/ospf_vty.h"

#define ZAPD_DEFAULT_CONFIG   "zapd.conf"

// #define ZEBRA_ROUTE_ZAP                  9
// #define ZEBRA_ROUTE_MAX                  10

/* ospfd privileges */
zebra_capabilities_t _caps_p [] = 
{
  ZCAP_NET_RAW,
  ZCAP_BIND,
  ZCAP_NET_ADMIN,
};

struct zebra_privs_t ospfd_privs =
{
#if defined(QUAGGA_USER) && defined(QUAGGA_GROUP)
  .user = QUAGGA_USER,
  .group = QUAGGA_GROUP,
#endif
#if defined(VTY_GROUP)
  .vty_group = VTY_GROUP,
#endif
  .caps_p = _caps_p,
  .cap_num_p = sizeof(_caps_p)/sizeof(_caps_p[0]),
  .cap_num_i = 0
};

/* Configuration filename and directory. */
char config_default[] = SYSCONFDIR ZAPD_DEFAULT_CONFIG;

/* OSPFd options. */
struct option longopts[] = 
{
  { "daemon",      no_argument,       NULL, 'd'},
  { "config_file", required_argument, NULL, 'f'},
  { "help",        no_argument,       NULL, 'h'},
  { "vty_port",    required_argument, NULL, 'P'},
  { "version",     no_argument,       NULL, 'v'},
  { 0 }
};

/* OSPFd program name */

/* Master of threads. */
struct thread_master *master;

/* Process ID saved for use by init system */
const char *pid_file = PATH_OSPFD_PID;

#ifdef SUPPORT_OSPF_API
extern int ospf_apiserver_enable;
#endif /* SUPPORT_OSPF_API */

/* Help information display. */
static void __attribute__ ((noreturn))
usage (char *progname, int status)
{
  if (status != 0)
    fprintf (stderr, "Try `%s --help' for more information.\n", progname);
  else
    {    
      printf ("Usage : %s [OPTION...]\n\
Daemon which manages ZAP.\n\n\
-d, --daemon       Runs in daemon mode\n\
-f, --config_file  Set configuration file name\n\
-P, --vty_port     Set vty's port number\n\
-v, --version      Print program version\n\
-h, --help         Display this help and exit\n\
\n\
Report bugs to %s\n", progname, ZEBRA_BUG_ADDRESS);
    }
  exit (status);
}

/* SIGHUP handler. */
static void 
sighup (void)
{
  zlog (NULL, LOG_INFO, "SIGHUP received");
}

/* SIGINT / SIGTERM handler. */
static void
sigint (void)
{
  zlog_notice ("Terminating on signal");
  ospf_terminate ();
}

/* SIGUSR1 handler. */
static void
sigusr1 (void)
{
  zlog_rotate (NULL);
}

struct quagga_signal_t ospf_signals[] =
{
  {
    .signal = SIGHUP,
    .handler = &sighup,
  },
  {
    .signal = SIGUSR1,
    .handler = &sigusr1,
  },  
  {
    .signal = SIGINT,
    .handler = &sigint,
  },
  {
    .signal = SIGTERM,
    .handler = &sigint,
  },
};


extern struct zclient* zclient = NULL;

/* Inteface addition message from zebra. */
int
zap_interface_add (int command, struct zclient *zclient, zebra_size_t length)
{
  struct interface *ifp;
 
   ifp = zebra_interface_add_read(zclient->ibuf);
   
   zlog_info ("Zebra: ifp=0x%x ", (unsigned int)ifp);
   // if (!(ifp = zebra_interface_state_read(zclient->ibuf)))
   // return 0;
   
 // char ifname_tmp[INTERFACE_NAMSIZ];

  /* Read interface name. */
 // stream_get (ifname_tmp, zclient->ibuf, INTERFACE_NAMSIZ);

  /* Lookup/create interface by name. */
 // ifp = if_get_by_name_len (ifname_tmp, strnlen(ifname_tmp, INTERFACE_NAMSIZ));



  
/*  zlog_info ("Zebra: interface add %s index %d flags %d metric %d mtu %d",
	       ifp->name, ifp->ifindex, ifp->flags, ifp->metric, ifp->mtu); */
	       
  zlog_info ("Zebra: interface add ");
	       
	   /*    
  if (ifp == NULL) {
  	zlog_info ("ERROR");
  	} else {
  	
  	 zlog_info ("Zebra: interface add %s ", ifp->name);
	       
zlog_info ("Zebra: interface add"); 
  	}
     
 */
	       
 return 0;

}


void
zap_init ()
{
  int i;
  zclient = zclient_new ();
  zclient_init (zclient, ZEBRA_ROUTE_ZAP);
  
 for(i=0; i++ < ZEBRA_ROUTE_MAX;)
  if( i != ZEBRA_ROUTE_ZAP)
	zclient_redistribute (ZEBRA_REDISTRIBUTE_ADD, zclient, i);
//    zclient_redistribute_set (zclient, i);
    
  zclient->interface_add = zap_interface_add;

}


/* ZAPD main routine. */
int
main (int argc, char **argv)
{
  char *p;
  char *vty_addr = NULL;
  int vty_port = 26666;
  int daemon_mode = 0;
  char *config_file = NULL;
  char *progname;
  struct thread thread;
  int dryrun = 0;

  /* Set umask before anything for security */
  umask (0027);

  /* get program name */
  progname = ((p = strrchr (argv[0], '/')) ? ++p : argv[0]);

  /* Invoked by a priviledged user? -- endo. */
  if (geteuid () != 0)
    {
      errno = EPERM;
      perror (progname);
      exit (1);
    }

  zlog_default = openzlog (progname, ZLOG_OSPF,
			   LOG_CONS|LOG_NDELAY|LOG_PID, LOG_DAEMON);

  /* OSPF master init. */
  ospf_master_init ();

#ifdef SUPPORT_OSPF_API
  /* OSPF apiserver is disabled by default. */
  ospf_apiserver_enable = 0;
#endif /* SUPPORT_OSPF_API */

  while (1) 
    {
      int opt;

      opt = getopt_long (argc, argv, "dlf:hP:v", longopts, 0);
    
      if (opt == EOF)
	break;

      switch (opt) 
	{
	case 0:
	  break;
	case 'd':
	  daemon_mode = 1;
	  break;
	case 'f':
	  config_file = optarg;
	  break;
	case 'A':
	  vty_addr = optarg;
	  break;
        case 'i':
          pid_file = optarg;
          break;
	case 'P':
          /* Deal with atoi() returning 0 on failure, and ospfd not
             listening on ospfd port... */
          if (strcmp(optarg, "0") == 0) 
            {
              vty_port = 0;
              break;
            } 
          vty_port = atoi (optarg);
          if (vty_port <= 0 || vty_port > 0xffff)
            vty_port = OSPF_VTY_PORT;
  	  break;
	case 'u':
	  ospfd_privs.user = optarg;
	  break;
	case 'g':
	  ospfd_privs.group = optarg;
	  break;
#ifdef SUPPORT_OSPF_API
	case 'a':
	  ospf_apiserver_enable = 1;
	  break;
#endif /* SUPPORT_OSPF_API */
	case 'v':
	  print_version (progname);
	  exit (0);
	  break;
	case 'C':
	  dryrun = 1;
	  break;
	case 'h':
	  usage (progname, 0);
	  break;
	default:
	  usage (progname, 1);
	  break;
	}
    }

  /* Initializations. */
  master = om->master;

  /* Library inits. */
  zprivs_init (&ospfd_privs);
  signal_init (master, Q_SIGC(ospf_signals), ospf_signals);
  cmd_init (1);
  debug_init ();
  vty_init (master);
  memory_init ();

  access_list_init ();
  prefix_list_init ();


//////////////////////////////////////////////////////////////

zap_init();

//////////////////////////////////////////////////////////////
  
  sort_node ();

  /* Get configuration file. */
  vty_read_config (config_file, config_default);

  /* Start execution only if not in dry-run mode */
  if (dryrun)
    return(0);
  
  /* Change to the daemon program. */
  if (daemon_mode && daemon (0, 0) < 0)
    {
      zlog_err("ZAPD daemon failed: %s", strerror(errno));
      exit (1);
    }

  /* Process id file create. */
  pid_output (pid_file);

  /* Create VTY socket */
  vty_serv_sock (vty_addr, vty_port, OSPF_VTYSH_PATH);

  /* Print banner. */
  zlog_notice ("ZAPD %s starting: vty@%d", QUAGGA_VERSION, vty_port);

  /* Fetch next active thread. */
  while (thread_fetch (master, &thread))
    thread_call (&thread);

  /* Not reached. */
  return (0);
}

