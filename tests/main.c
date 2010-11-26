#include <zebra.h>

#include "log.h"
#include "version.h"
#include <getopt.h>
#include "command.h"
#include "thread.h"
#include <signal.h>

/* just some includes, as usual */

/* you might want to put the following #defines on a zapd.h file 
 */

#define LDPD_DEFAULT_CONFIG "zapd.conf"
/* this will be the name of the config file in the zebra
 *  config directory (could be /usr/local/etc/)
 */

#define LDPD_VTY_PORT 26666
/* telnet to this port to login to the zapd vty
 */


#define LDPD_VTYSH_PATH "/tmp/.zapd"
/* name of the unix socket to communicate with the vtysh
 */


/*
 * *  GLOBALS
 */

char config_current[] = LDPD_DEFAULT_CONFIG;
char config_default[] = SYSCONFDIR LDPD_DEFAULT_CONFIG;
/* zebra does #define  SYSCONFDIR
 */

struct option longopts[] = 
{
  { "daemon",      no_argument,       NULL, 'd'},
  { "config_file", required_argument, NULL, 'f'},
  { "help",        no_argument,       NULL, 'h'},
  { "vty_port",    required_argument, NULL, 'P'},
  { "version",     no_argument,       NULL, 'v'},
  { 0 }
};

char* progname;
/* will contain (mangled) argv[0]
 */


struct thread_master *master;
/* needed by the thread implementation
 */





/* These 2 are defined somewhere else, say in libzapd.a
 */
#ifdef REALLY_DUMMY
void zap_init(void) {return;};
void zap_terminate(void) {return;};
#else
void zap_init(void);
void zap_terminate(void);
#endif



void zap_init() {


}

void zap_terminate() {


}



/* Help information display. */
static void
usage (int status)
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


int main(int argc, char** argv, char** envp) {
  char *p;
  int vty_port = 0;
  int daemon_mode = 0;
  char *config_file = NULL;
  struct thread thread;
          
  umask(0027);
 
 
  progname =  ((p = strrchr (argv[0], '/')) ? ++p : argv[0]);
 
 /*
  zlog_default = openzlog (progname, ZLOG_NOLOG, ZLOG_ZAP,
			   LOG_CONS|LOG_NDELAY|LOG_PID, LOG_DAEMON);
			   */
			   
  zlog_default = openzlog("testsig", ZLOG_NONE,
                          LOG_CONS|LOG_NDELAY|LOG_PID, LOG_DAEMON);

/* initialize the log subsystem, you will have to include
 * ZLOG_ZAP in the zlog_proto_t enum type definition in 
 * lib/log.h
 */

     
/* this while just reads the options */                       
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
	case 'P':
	  vty_port = atoi (optarg);
	  break;
	case 'v':
	  // print_version ();
	  exit (0);
	  break;
	case 'h':
	  usage (0);
	  break;
	default:
	  usage (1);
	  break;
	}
    }
                                                                                                                                                                                                                                                                                              
                                                                                                                                                                                                                                                                                              
 
/* one to control them all, ... */
  master = thread_master_create ();
  //signal_init (master, Q_SIGC(sigs), sigs);
/* this the main thread controlling structure,
 * nothing to remember.
 */


/* before you start the engine, put your safety belt on
 */


/* 
 * * Library inits.
 */
  cmd_init (1);
/* initializes the command sub-system, if arg, add some commands
 * which are mostly only useful for humans on the vty
 */

  //vty_init ();
  memory_init ();
  access_list_init ();
  prefix_list_init ();
/* these are all from libzebra
 */

              
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
  //vty_read_config (config_file, config_current, config_default);
/* read the config file, your commands should be defined before this
 */



  /* Change to the daemon program. */
  if (daemon_mode)
    daemon (0, 0);

  /* Create VTY socket */
 // vty_serv_sock (vty_port ? vty_port : LDPD_VTY_PORT, LDPD_VTYSH_PATH);
/* start the TCP and unix socket listeners
 */


  /* Print banner. */
  //zlog (NULL, LOG_INFO, "ZAP (%s) starts", ZEBRA_VERSION);
    
  /* Fetch next active thread. */
  while (thread_fetch (master, &thread))
    thread_call (&thread);
/* this is the main event loop */

/* never reached */
  return 0;
}


/* SIGHUP handler. */
void 
sighup (int sig)
{
  zlog (NULL, LOG_INFO, "SIGHUP received");
}

/* SIGINT handler. */
void
sigint (int sig)
{
  zlog (NULL, LOG_INFO, "Terminating on signal");

  zap_terminate ();
/* this is your clean-up function */

  exit (0);
}

/* SIGUSR1 handler. */
void
sigusr1 (int sig)
{
  zlog_rotate (NULL);
}

#define RETSIGTYPE void
/* Signal wrapper. */
RETSIGTYPE *
signal_set (int signo, void (*func)(int))
{
  int ret;
  struct sigaction sig;
  struct sigaction osig;

  sig.sa_handler = func;
  sigemptyset (&sig.sa_mask);
  sig.sa_flags = 0;
#ifdef SA_RESTART
  sig.sa_flags |= SA_RESTART;
#endif /* SA_RESTART */

  ret = sigaction (signo, &sig, &osig);

  if (ret < 0) 
    return (SIG_ERR);
  else
    return (osig.sa_handler);
}




/* zapd_main.c ends 
----------------------------------------------------------------
*/

