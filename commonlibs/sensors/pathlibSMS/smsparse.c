/* smsparse.c - parse SMS radar CAN messages
*/
#include <sys_os.h>
#include "sys_rt.h"
#include "local.h"
#include "timestamp.h"
//#include "data_log.h"
#include "db_clt.h"
#include "db_utils.h"
#include <sys/stat.h>
#include "smsparse.h"
#include "sms_lib.h"

#define TRACE		1
#define USE_DATABASE	2
#define MYSQL		4
int output_mask = 0;            // 1 trace, 2 DB server, 4 MySQL

unsigned short debug = 0;
unsigned char debug2 = 0;

int newsockfd;
db_clt_typ *pclt = NULL;
#define BUFSIZE	1500

static int sig_list[] = {
   SIGINT,
   SIGQUIT,
   SIGTERM,
   ERROR,
};

static jmp_buf exit_env;

static void sig_hand(int code)
{
   longjmp(exit_env, code);
}

smserr_t smserr;
sms_id_err_t smsiderr;
sensctl_typ sensctl;
objlist_typ objlist;
smsobjarr_typ smsarr[SMSARRMAX];
unsigned int cyc_cnt_sav = 0;
int objindex = 0;

rcv_msg_hdr_t rcv_msg_hdr;
objctl_typ objctl;
smsobj_typ object[MAXOBJ];
timestamp_t global_ts;

// All variables used in messages are created
static db_id_t db_vars_list[] = {
   {DB_SMS_OBJARR0_VAR, sizeof(smsobjarr_typ)},
   {DB_SMS_OBJARR1_VAR, sizeof(smsobjarr_typ)},
   {DB_SMS_OBJARR2_VAR, sizeof(smsobjarr_typ)},
   {DB_SMS_OBJARR3_VAR, sizeof(smsobjarr_typ)},
   {DB_SMS_OBJARR4_VAR, sizeof(smsobjarr_typ)},
   {DB_SMS_OBJARR5_VAR, sizeof(smsobjarr_typ)},
   {DB_SMS_OBJARR6_VAR, sizeof(smsobjarr_typ)},
   {DB_SMS_OBJARR7_VAR, sizeof(smsobjarr_typ)},
   {DB_SMS_OBJARR8_VAR, sizeof(smsobjarr_typ)},
   {DB_SMS_OBJARR9_VAR, sizeof(smsobjarr_typ)},
   {DB_SMS_OBJARR10_VAR, sizeof(smsobjarr_typ)},
   {DB_SMS_OBJARR11_VAR, sizeof(smsobjarr_typ)},
   {DB_SMS_OBJARR12_VAR, sizeof(smsobjarr_typ)},
   {DB_SMS_OBJARR13_VAR, sizeof(smsobjarr_typ)},
   {DB_SMS_OBJARR14_VAR, sizeof(smsobjarr_typ)},
   {DB_SMS_OBJARR15_VAR, sizeof(smsobjarr_typ)},
   {DB_SMS_CMD_VAR, sizeof(sndmsg_t)},
   {DB_SMS_CMDCTR_VAR, sizeof(int)},
   {DB_SMS_OBJLIST_VAR, sizeof(objlist_typ)},
   {DB_SMS_SENSCTL_VAR, sizeof(sensctl_typ)},
   {DB_SMS_ERR_VAR, sizeof(smserr_t)},
   {DB_SMS_ID_ERR_VAR, sizeof(sms_id_err_t)},
   {DB_SMS_NUM_SAVED_OBJS_VAR, sizeof(char)}
};

#define NUM_DB_VARS    sizeof(db_vars_list)/sizeof(db_id_t)

int main(int argc, char *argv[])
{
   int status = 1;
   int option;
   int retval;
   int index = 0;
   char *local_ip = "172.16.1.50";
   char *remote_ip;
   int i;
   char *ipbytestr;
   unsigned char tscan_threshold = 60;
   unsigned short socket_timeout = 10000;       //socket timeout in msec

   char hostname[MAXHOSTNAMELEN + 1];
   char *domain = DEFAULT_SERVICE;      //for QNX6, use e.g. "ids"
   int xport = COMM_PSX_XPORT;  //for QNX6, no-op

   char buf[BUFSIZE];
   sms_can_msg_t message;
   smscmd_t command;
   char *pcommand;
   int cmdctr = 0;
   int ctr = 0;
   int localcmdctr = 0;
   sndmsg_t sndmsg;
   int simulation_mode = 0;
   int create_db_vars = 1;
   int use_stdin = 0;
   struct timespec tspec;
   struct timeval tv;
   fd_set myfdset;

   remote_ip = strdup("172.16.1.200");

   while ((option = getopt(argc, argv, "d:o:sl:r:t:k:chp")) != EOF) {
      switch (option) {
      case 'd':
         debug = (unsigned short) atoi(optarg);
         break;
      case 'o':
         output_mask = atoi(optarg);
         break;
      case 's':
         simulation_mode = 4;
         break;
      case 'l':
         local_ip = strdup(optarg);
         break;
      case 'r':
         remote_ip = strdup(optarg);
         break;
      case 't':
         tscan_threshold = (unsigned char) atoi(optarg);
         break;
      case 'k':
         socket_timeout = (unsigned short) atoi(optarg);
         printf("socket_timeout %d msec\n", socket_timeout);
         fflush(stdout);
         break;
      case 'c':
         create_db_vars = 0;
         break;
      case 'p':
         use_stdin = 1;
         break;
      case 'h':
      default:
         printf("Usage: %s -d (debug,  1=data receive 2=text to message return\n\t\t\t\t\t4=check head & tail 8=text to message \n\t\t\t\t\t16=sensor control message \n\t\t\t\t\t32=object control message \n\t\t\t\t\t64=parsed data 128=command sent)\n\t\t\t    -o (output mask, 1=trace file 2=use DB 4=MySQL)\n\t\t\t    -f < \"input file\"\n\t\t\t    -s (simulation mode) -l <local IP> -r <remote IP (bumper box) -t <t_scan threshold> -k <socket timeout (ms)> -c (don't create db variables)\n", argv[0]);
         exit(EXIT_FAILURE);
         break;
      }
   }

   /* Initialize command structure */
   memset(&command, 0, sizeof(smscmd_t));
   printf("remote_ip string %s as integers", remote_ip);
   ipbytestr = strtok(remote_ip, ".");
   for (i = 0; i < 4; i++) {
      command.ip[i] = atoi(ipbytestr);
      printf(" %hhu", command.ip[i]);
      ipbytestr = strtok(NULL, ".");
   }
   printf("\n");
   fflush(stdout);
   sprintf(&command.trainseq[0], "%c%c%c%c%c%c%c%c%c", 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4, 0xC4);
   command.numpkts = 1;
   command.pktno = 0;
   command.canno = 0;
   command.CAN_ID = htons(0x03F2);
   command.cmd_data.cmdtarg = 0;
   command.cmd_data.data[0] = 0;
   command.cmd_data.data[1] = 0;
   command.cmd_data.data[2] = 0;
   command.cmd_data.data[3] = 64;
   command.cmd_data.value = simulation_mode;
   sprintf(&command.tailseq[0], "%c%c%c%c%c%c%c%c%c", 0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7);
   if (!use_stdin) {
      newsockfd = OpenSMSConnection(local_ip, remote_ip);
      if (newsockfd < 0) {
         print_err("OpenSMSConnection failed");
         exit(EXIT_FAILURE);
      }
   } else
      newsockfd = STDIN_FILENO;

   if (debug) {
      print_err("OpenSMSConnection succeeded");
   }
   if (output_mask & USE_DATABASE) {
      fprintf(stdout, "Using database\n");
      fflush(stdout);
      get_local_name(hostname, MAXHOSTNAMELEN);
      if (create_db_vars) {
         if ((pclt = db_list_init(argv[0], hostname, domain, xport, db_vars_list, NUM_DB_VARS, NULL, 0)) == NULL) {
            exit(EXIT_FAILURE);
         }
      } else {
         if ((pclt = db_list_init(argv[0], hostname, domain, xport, NULL, 0, NULL, 0)) == NULL) {
            exit(EXIT_FAILURE);
         }
      }
   }
   
   // Initialize structures
   memset(&sensctl, 0, sizeof(sensctl));
   memset(&objlist, 0, sizeof(objlist));
   memset(smsarr, 0, sizeof(smsobjarr_typ)*SMSARRMAX);
   memset(&smserr, 0, sizeof(smserr));
   memset(&smsiderr, 0, sizeof(smsiderr));
   smserr.tscan_threshold = tscan_threshold;
   smserr.socket_timeout = socket_timeout;
   
   if (output_mask & USE_DATABASE)     
      db_clt_write(pclt, DB_SMS_ERR_VAR, sizeof(smserr_t), &smserr);
        

   if (setjmp(exit_env) != 0) {
      if (pclt != 0) {
         if (create_db_vars)
            db_list_done(pclt, db_vars_list, NUM_DB_VARS, NULL, 0);
         else
            clt_logout(pclt);
      }
      close(newsockfd);
      fflush(NULL);
      exit(EXIT_SUCCESS);
   } else
      sig_ign(sig_list, sig_hand);

   // Set simulation mode
   if (simulation_mode == 4)
      smscommand(&command, 0, 0, 0, 64, simulation_mode, newsockfd);
   pcommand = (char *) &command;
   for (index = 0; index < sizeof(smscmd_t); index++)
      printf("%#0X ", pcommand[index] & 0xFF);
   printf("\n\n");
   fflush(stdout);

   while (1) {
      FD_ZERO(&myfdset);
      FD_SET(newsockfd, &myfdset);
      tv.tv_usec = smserr.socket_timeout * 1000;
      tv.tv_sec = 0;
      retval = select(newsockfd + 1, &myfdset, NULL, NULL, &tv);
      if (retval <= 0) {
         smserr.socket_err_cnt++;
         clock_gettime(CLOCK_REALTIME, &tspec);
         smserr.last_socket_err_time = (time_t) tspec.tv_sec;
         if (output_mask & USE_DATABASE)
            db_clt_write(pclt, DB_SMS_ERR_VAR, sizeof(smserr_t), &smserr);
         continue;
      } else if (retval < 0) {
         print_err("Select() error\n");
         return -1;
      } else {
         if ((retval = read(newsockfd, buf, BUFSIZE)) < 0) {
         //   system("/home/capath/paws/test/reboot_sms.sh");
            print_err("read");
            if (!use_stdin) {
               close(newsockfd);
               newsockfd = OpenSMSConnection(local_ip, remote_ip);
               if (newsockfd < 0) {
                  print_err("OpenSMSConnection failed");
               }
            }
            continue;
         }
         if (debug & CASE_RCV)
            print_log(CASE_RCV, buf, NULL, NULL, NULL, NULL);
         if ((retval >= 48) && (retval < BUFSIZE)) {
            if ((status = check_head_tail(retval, &rcv_msg_hdr, buf, debug))
                == EOF) {
               print_err("check_head_tail");
               continue;
            }

            for (index = 29; index < (retval - 9); index += 10) {
               if ((status = text_to_msg(index, &message, buf, debug)) == EOF) {
                  print_err("text_to_msg");
                  continue;
               }
               if (debug & CASE_TEXT_TO_MSG_RETURN)
                  print_log(CASE_TEXT_TO_MSG_RETURN, &message, NULL, NULL, NULL, NULL);
               parse_msg(&message);
               if (output_mask & USE_DATABASE) {
                  if ((++ctr % 5) == 0) {
                     if (db_clt_read(pclt, DB_SMS_CMDCTR_VAR, sizeof(int), &cmdctr) == 0) {
                        print_err("Cannot read SMS command counter");
                     } else {
                        if (cmdctr > localcmdctr) {
                           if (db_clt_read(pclt, DB_SMS_CMD_VAR, sizeof(sndmsg_t), &sndmsg) == 0) {
                              print_err("Cannot read SMS command counter");
                              ctr = 0;
                           } else {
                              printf("SMSPARSE.C: Got SMS command\n");
                              fflush(stdout);
                              localcmdctr = cmdctr;
                              smscommand(&command, sndmsg.data[0], sndmsg.data[1], sndmsg.data[2], sndmsg.data[3], sndmsg.value, newsockfd);
                           }
                        }
                     }
                  }             //end of if( (++ctr % 5) == 0)
               }                //end of if (output_mask & USE_DATABASE)
            }                   //end of for loop
            
            if (output_mask & USE_DATABASE) {
               db_clt_write(pclt, DB_SMS_SENSCTL_VAR, sizeof(sensctl_typ), &sensctl);
               db_clt_write(pclt, DB_SMS_ERR_VAR, sizeof(smserr_t), &smserr);
               db_clt_write(pclt, DB_SMS_ID_ERR_VAR, sizeof(sms_id_err_t), &smsiderr);                 
               db_clt_write(pclt, DB_SMS_OBJLIST_VAR, sizeof(objlist_typ), &objlist);
               
               for(i = DB_SMS_OBJARR0_VAR; i < DB_SMS_OBJARR0_VAR + SMSARRMAX; i++)
                  db_clt_write(pclt, i, sizeof(smsobjarr_typ), &smsarr[i-DB_SMS_OBJARR0_VAR]);
               /*
               objlist_typ objlisttemp;
               memset(&objlisttemp, 0, sizeof(objlisttemp));
               
               db_clt_read(pclt, DB_SMS_OBJLIST_VAR, sizeof(objlist_typ), &objlisttemp);
               
               if (objlisttemp.num_obj <= 0)
                  fprintf(stderr, "%u %d\n", objlisttemp.cyc_cnt, objlisttemp.num_obj);
               
               for(i = 0; i < objlisttemp.num_obj; i++) {
                  unsigned char sms_obj_id = objlisttemp.arr[i];
                  int db_arr = DB_VAR_SMS_OBJ_INDEX(sms_obj_id);
                  int db_var = DB_VAR_SMS_OBJ(sms_obj_id);
                  unsigned int cyc_cnttemp = objlisttemp.cyc_cnt;
                
                  smsobjarr_typ smstemp;
                  memset(&smstemp, 0, sizeof(smstemp));
                   
                  db_clt_read(pclt, db_var, sizeof(smsobjarr_typ), &smstemp);
                  
                  fprintf(stderr, "%d-%d %d %d ol-%u ol-%hhd o-%u o-%hhd %.1f %.1f %.1f %.1f %.1f\n", db_var, db_arr, objlisttemp.num_obj, i, cyc_cnttemp, sms_obj_id, smstemp.object[db_arr].cyc_cnt, smstemp.object[db_arr].obj, .1*smstemp.object[db_arr].xrange, .1*smstemp.object[db_arr].yrange, .1*smstemp.object[db_arr].xvel, .1*smstemp.object[db_arr].yvel, .2*smstemp.object[db_arr].length); 
               } */            
            } 
         } else if (retval > 0) {
            fprintf(stderr, "Read error for stdin:  read %d bytes\n", retval);
            fflush(stderr);
         }
      }
   }
   return -1;
}

int parse_msg(sms_can_msg_t * message)
{
   unsigned char obj;
   int arr;
   int arrmember;
   int i;
   struct timespec tmspec;
   struct tm *timestamp;
   static FILE *tracefd;
   char num_saved_objs = 0;
   char errbuff[1024];


//Get timestamp
   if (clock_gettime(CLOCK_REALTIME, &tmspec) < 0) {
      print_err("timestamp");
   }
   switch (message->CAN_ID) {
   case 0x500:
      memset(&sensctl, 0, sizeof(sensctl));
      
      sensctl.bump_d_net = message->data[0];
      sensctl.canstat = message->data[1];
      sensctl.sensorspres = (unsigned short) ((message->data[2] & 0X00FF) | ((message->data[3] << 8) & 0X0F00));
      sensctl.enetstat = (message->data[3] >> 4) & 0XFF;
      sensctl.timestamp = (unsigned int)
          ((message->data[4] & 0X000000FF) +    //timestamp 1
           ((message->data[5] << 8) & 0X0000FF00) +     //timestamp 2
           ((message->data[6] << 16) & 0X00FF0000) +    //timestamp 3
           ((message->data[7] << 24) & 0XFF000000)      //timestamp 4
          );
      if (debug & CASE_SENSOR_CTL)
         print_log(CASE_SENSOR_CTL, message, &sensctl, NULL, NULL, NULL);
//printf("%s", ctime(&tmspec.tv_sec));
//printf("sensctl.timestamp %u\n", sensctl.timestamp);
      //CANSTAT error
      if (sensctl.canstat & BAD_CAN_STAT_MASK) {
         smserr.canstat_err_cnt++;
         smserr.last_canstat_err_time = (time_t) tmspec.tv_sec;
         smserr.last_canstat = sensctl.canstat;
      }
      // SENSORPRES error
      for (i = 0; i < NO_HEADS; i++) {
         if ((sensctl.sensorspres & (1 << i)) == 0) {
            smsiderr.id_err_cnt[i]++;
            smsiderr.last_id_err_time[i] = (time_t) tmspec.tv_sec;
         }
      }
      if (debug & CASE_ERR) {
         sms_sprint_err(errbuff, &smserr, &smsiderr);
         fprintf(stderr, "%s\n", errbuff);
         fflush(stderr);

      }

      break;
   case 0x501:
      objindex = 0;
      memset(&objctl, 0, sizeof(objctl));
      memset(smsarr, 0, sizeof(smsobjarr_typ)*SMSARRMAX);
      
      objctl.no_obj = message->data[0]; // number of objects
      objctl.no_msgs = message->data[1];        // number of messages
      objctl.t_scan = message->data[2]; // this cycle time (ms)

      objctl.mode = message->data[3];   // mode (0=normal tracking,
      // 1=simulation)
      objctl.cyc_cnt = (unsigned int)
          ((message->data[4] & 0X000000FF) +    //curr cyc cnt B0
           ((message->data[5] << 8) & 0X0000FF00) +     //byte 1
           ((message->data[6] << 16) & 0X00FF0000) +    //byte 2
           ((message->data[7] << 24) & 0XFF000000)      //byte 3
          );

      // Zero objlist and then set fields common to objctl
      memset(&objlist, 0, sizeof(objlist));
      get_current_timestamp(&objlist.ts);       // start of object list
      objlist.mode = objctl.mode;
      objlist.t_scan = objctl.t_scan;
      objlist.num_obj = objctl.no_obj;  // number of objects
      objlist.cyc_cnt = objctl.cyc_cnt;

      // NO. SAVED < TRACKED OBJ error
      // Note: the number of saved objects is not set by smsparse.  It's set,
      // and written to the database, by wrfiles.
      if (output_mask & USE_DATABASE) 
         db_clt_read(pclt, DB_SMS_NUM_SAVED_OBJS_VAR, sizeof(char), &num_saved_objs);
      smserr.num_saved_obj = num_saved_objs;
      if (objctl.no_obj > smserr.num_saved_obj) {
         smserr.num_saved_obj_err_cnt++;
         smserr.last_num_saved_err_time = (time_t) tmspec.tv_sec;
         smserr.last_num_tracked_obj = objctl.no_obj;
      }
      // T_SCAN error
      if ((unsigned char) objctl.t_scan >= smserr.tscan_threshold) {
         smserr.tscan_err_cnt++;
         smserr.last_tscan_err_time = (time_t) tmspec.tv_sec;
         smserr.last_tscan = objctl.t_scan;
      }
      // CYCLE COUNT error
      if (((unsigned int) objctl.cyc_cnt - cyc_cnt_sav) != 1) {
         smserr.cyc_cnt_err_cnt++;
         smserr.last_cyc_cnt_err_time = (time_t) tmspec.tv_sec;
         smserr.last_cyc_cnt[0] = cyc_cnt_sav;
         smserr.last_cyc_cnt[1] = objctl.cyc_cnt;
      }
      cyc_cnt_sav = objctl.cyc_cnt;

      if (debug & CASE_OBJECT_CTL) {
         sms_print_obj_list(message, &objctl, &objlist);
      }
      if (debug & CASE_ERR) {
         sms_sprint_err(errbuff, &smserr, &smsiderr);
         fprintf(stderr, "%s\n", errbuff);
         fflush(stderr);
      }
      break;
   case 0x03F2:
#define READ_INT	8
#define READ_FLOAT	9
      printf("cmd_type: command message type\n");
      if (message->data[2] == READ_INT) {
         printf("READPARM: Parameter %d integer value is %d\n", message->data[1], (int) ((message->data[4] & 0X000000FF) +      //integer parameter byte 1
                                                                                         ((message->data[5] << 8) & 0X0000FF00) +       //integer parameter byte 2
                                                                                         ((message->data[6] << 16) & 0X00FF0000) +      //integer parameter byte 3
                                                                                         ((message->data[7] << 24) & 0XFF000000))       //integer parameter byte 4
             );
         printf("READPARM: Parameter %d bytes are %#x %#x %#x %#x\n", message->data[1], message->data[7],       //float parameter byte 1
                message->data[6],       //float parameter byte 2
                message->data[5],       //float parameter byte 3
                message->data[4]        //float parameter byte 4
             );
      } else if (message->data[2] == READ_FLOAT) {
         float tempfloat;
         int tempint;
         tempint = ((message->data[4] & 0X000000FF) +   //float parameter byte 1
                    ((message->data[5] << 8) & 0X0000FF00) +    //float parameter byte 2
                    ((message->data[6] << 16) & 0X00FF0000) +   //float parameter byte 3
                    ((message->data[7] << 24) & 0XFF000000));   //float parameter byte 4
         tempfloat = tempint / 1000000.0;
         printf("READPARM: Parameter %d float value is %f\n", message->data[1], tempfloat);
         printf("READPARM: Parameter %d bytes are %#x %#x %#x %#x\n", message->data[1], message->data[7],       //float parameter byte 1
                message->data[6],       //float parameter byte 2
                message->data[5],       //float parameter byte 3
                message->data[4]        //float parameter byte 4
             );
      } else
         printf("READPARM: Could not read parameter\n");
      fflush(stdout);
      break;
   default:
      if (message->CAN_ID >= 0x510 && message->CAN_ID <= 0x5FF) {

         obj = (unsigned char) (((message->data[7]) >> 2) & 0X3F);
         arr = obj / MAXDBOBJS;
         arrmember = obj % MAXDBOBJS;

         //Get timestamp
         if (clock_gettime(CLOCK_REALTIME, &tmspec) < 0) {
            print_err("timestamp");
         } else {
            timestamp = localtime((time_t *) & tmspec.tv_sec);
            smsarr[arr].object[arrmember].smstime.hour = timestamp->tm_hour;
            smsarr[arr].object[arrmember].smstime.min = timestamp->tm_min;
            smsarr[arr].object[arrmember].smstime.sec = timestamp->tm_sec;
            smsarr[arr].object[arrmember].smstime.millisec = tmspec.tv_nsec / 1000000;
         }

         // doesn't matter if smsarr is ever cleared,
         // only object IDs in the object list arrays
         // are valid, and new data is written only for them 
         objlist.arr[objindex++] = obj;
         smsarr[arr].object[arrmember].obj = obj;
         smsarr[arr].object[arrmember].xrange = (short) (((message->data[0]) + ((message->data[1] & 0X3F) << 8)) - 8192);
         smsarr[arr].object[arrmember].yrange = (short) (((((message->data[1] & 0XC0) >> 6) + (message->data[2] << 2) + ((message->data[3] & 0X0F) << 10)) - 8192));
         smsarr[arr].object[arrmember].xvel = (short) (((((message->data[3] & 0XF0) >> 4) + (((message->data[4] & 0X7F)) << 4)) - 1024));
         smsarr[arr].object[arrmember].yvel = (short) (((((message->data[4] & 0X80) >> 7) + (((message->data[5])) << 1) + (((message->data[6] & 0X03)) << 9)) - 1024));
         smsarr[arr].object[arrmember].length = (short) (((message->data[6] >> 2) & 0X3F) + ((message->data[7] << 6) & 0XC0));
         smsarr[arr].object[arrmember].cyc_cnt = objctl.cyc_cnt;
         /*
         fprintf(stderr, "%d\n", objctl.cyc_cnt);
         fprintf(stderr, "%#hhx %#hhx %#hhx %#hhx %#hhx %#hhx %#hhx %#hhx\n", message->data[7], message->data[6], message->data[5], message->data[4], message->data[3], message->data[2], message->data[1], message->data[0]
             );
         fprintf(stderr, "%.1f %.1f %.1f %.1f %.1f\n", smsarr[arr].object[arrmember].xrange, smsarr[arr].object[arrmember].yrange, smsarr[arr].object[arrmember].xvel, smsarr[arr].object[arrmember].yvel, smsarr[arr].object[arrmember].length);
         fprintf(stderr, "%.1lf %.1lf %.1lf %.1lf %.1lf\n", smsarr[arr].object[arrmember].xrange, smsarr[arr].object[arrmember].yrange, smsarr[arr].object[arrmember].xvel, smsarr[arr].object[arrmember].yvel, smsarr[arr].object[arrmember].length);
         if ((fabs(smsarr[arr].object[arrmember].xrange) > 2000) || (fabs(smsarr[arr].object[arrmember].yrange) > 2000) || (fabs(smsarr[arr].object[arrmember].xvel) > 200) || (fabs(smsarr[arr].object[arrmember].yvel) > 200) || (fabs(smsarr[arr].object[arrmember].length) > 200)) {
            fprintf(stderr, "Range error in object floating point representations.\n");
            fprintf(stderr, "Data byte, in hex, from MSB to LSB:\n");
            fprintf(stderr, "%#hhx %#hhx %#hhx %#hhx %#hhx %#hhx %#hhx %#hhx\n", message->data[7], message->data[6], message->data[5], message->data[4], message->data[3], message->data[2], message->data[1], message->data[0]
                );
         }*/
         if (debug & CASE_OBJECT_DATA) {
            sms_print_obj_data(message, obj, &smsarr[0], arr, arrmember, objindex);
         }
         
         if (output_mask & TRACE) {

            if (tracefd == NULL) {
               tracefd = fopen("sms.out", "w");
               if (tracefd == NULL) {
                  print_err("trace file:");
                  fflush(stdout);
                  exit(EXIT_FAILURE);
               }
            }
            fprintf(tracefd, "%2.2d:%2.2d:%2.3f %4d%2.2d%2.2d ", timestamp->tm_hour, timestamp->tm_min, timestamp->tm_sec + tmspec.tv_nsec / 1000000000.0, timestamp->tm_year + 1900, timestamp->tm_mon + 1, timestamp->tm_mday);
            fprintf(tracefd, "%2.2d %4.1f %4.1f %4.1f %4.1f %4.1f ", obj, smsarr[arr].object[arrmember].xrange, smsarr[arr].object[arrmember].yrange, smsarr[arr].object[arrmember].xvel, smsarr[arr].object[arrmember].yvel, smsarr[arr].object[arrmember].length);
            fprintf(tracefd, "%#0x %#0x %#0x %d %d %u %d %u\n", sensctl.enetstat & 0xFF, sensctl.canstat & 0xFF, sensctl.sensorspres & 0xFFF, objctl.no_obj, objctl.no_msgs, objctl.t_scan & 0xFF, objctl.mode, objctl.cyc_cnt);
         }


      } else if (message->CAN_ID >= 0x700 && message->CAN_ID <= 0x7c0) {
         printf("Sensor %#x message: ", message->CAN_ID);
         for (i = 0; i < 8; i++)
            printf("%#0x ", message->data[i]);
         printf("\n");
      } else if (debug)
         printf("EXT_CAN: Unknown message type %#x (default \
case)\n", message->CAN_ID);
      fflush(stdout);
      break;
   }
   return 0;
}
