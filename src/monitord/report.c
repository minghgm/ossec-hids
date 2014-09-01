/* @(#) $Id: ./src/monitord/report.c, 2011/09/08 dcid Exp $
 */

/* Copyright (C) 2010 Trend Micro Inc.
 * All rights reserved.
 *
 * This program is a free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */


#include "shared.h"



/* print help statement */
void help_reportd()
{
    print_header();
    print_out("  Generate reports (via stdin)");
    print_out("  %s: -[Vhdtns] [-u user] [-g group] [-D dir] [-f filter value] [-r filter value]", ARGV0);
    print_out("    -V          Version and license message");
    print_out("    -h          This help message");
    print_out("    -d          Execute in debug mode. This parameter");
    print_out("                can be specified multiple times");
    print_out("                to increase the debug level.");
    print_out("    -t          Test configuration");
    print_out("    -n          Create description for the report");
    print_out("    -s          Show the alert dump");
    print_out("    -u <user>   Run as 'user'");
    print_out("    -g <group>  Run as 'group'");
    print_out("    -D <dir>    Chroot to 'dir'");
    print_out("    -f <filter> <value> Filter the results");
    print_out("    -r <filter> <value> Show related entries");
    print_out("    Filters allowed: group, rule, level, location,");
    print_out("                     user, srcip, filename");
    print_out("  Examples:");
    print_out("     -f group authentication_success (to filter on login success)");
    print_out("     -f level 10 (to filter on level >= 10)");
    print_out("     -f group authentication -r user srcip (to show srcip for all users)");
    print_out(" ");
    exit(1);
}



int main(int argc, char **argv)
{
    int c, test_config = 0;
    int uid=0,gid=0;
    char *dir  = DEFAULTDIR;
    char *user = USER;
    char *group = GROUPGLOBAL;

    char *filter_by = NULL;
    char *filter_value = NULL;

    char *related_of = NULL;
    char *related_values = NULL;
    report_filter r_filter;


    /* Setting the name */
    OS_SetName(ARGV0);

    r_filter.group = NULL;
    r_filter.rule = NULL;
    r_filter.level = NULL;
    r_filter.location = NULL;
    r_filter.srcip = NULL;
    r_filter.user = NULL;
    r_filter.files = NULL;
    r_filter.show_alerts = 0;

    r_filter.related_group = 0;
    r_filter.related_rule = 0;
    r_filter.related_level = 0;
    r_filter.related_location = 0;
    r_filter.related_srcip = 0;
    r_filter.related_user = 0;
    r_filter.related_file = 0;

    r_filter.report_name = NULL;

    while((c = getopt(argc, argv, "Vdhstu:g:D:f:v:n:r:")) != -1)
    {
        switch(c){
            case 'V':
                print_version();
                break;
            case 'h':
                help_reportd();
                break;
            case 'd':
                nowDebug();
                break;
            case 'n':
                if(!optarg)
                    ErrorExit("%s: -n needs an argument",ARGV0);
                r_filter.report_name = optarg;
                break;
            case 'r':
                if(!optarg || !argv[optind])
                    ErrorExit("%s: -r needs two argument",ARGV0);
                related_of = optarg;
                related_values = argv[optind];

                if(os_report_configfilter(related_of, related_values,
                                          &r_filter, REPORT_RELATED) < 0)
                {
                    ErrorExit(CONFIG_ERROR, ARGV0, "user argument");
                }
                optind++;
                break;
            case 'f':
                if(!optarg)
                    ErrorExit("%s: -f needs two argument",ARGV0);
                filter_by = optarg;
                filter_value = argv[optind];

                if(os_report_configfilter(filter_by, filter_value,
                                          &r_filter, REPORT_FILTER) < 0)
                {
                    ErrorExit(CONFIG_ERROR, ARGV0, "user argument");
                }
                optind++;
                break;
            case 'u':
                if(!optarg)
                    ErrorExit("%s: -u needs an argument",ARGV0);
                user=optarg;
                break;
            case 'g':
                if(!optarg)
                    ErrorExit("%s: -g needs an argument",ARGV0);
                group=optarg;
                break;
            case 'D':
                if(!optarg)
                    ErrorExit("%s: -D needs an argument",ARGV0);
                dir=optarg;
                break;
            case 't':
                test_config = 1;
                break;
            case 's':
                r_filter.show_alerts = 1;
                break;
            default:
                help_reportd();
                break;
        }

    }

    /* Starting daemon */
    debug1(STARTED_MSG,ARGV0);

    /* Check if the user/group given are valid */
    uid = Privsep_GetUser(user);
    gid = Privsep_GetGroup(group);
    if((uid < 0)||(gid < 0))
        ErrorExit(USER_ERROR,ARGV0,user,group);



    /* Exit here if test config is set */
    if(test_config)
        exit(0);


    /* Privilege separation */
    if(Privsep_SetGroup(gid) < 0)
        ErrorExit(SETGID_ERROR,ARGV0,group);


    /* chrooting */
    if(Privsep_Chroot(dir) < 0)
        ErrorExit(CHROOT_ERROR,ARGV0,dir);

    nowChroot();



    /* Changing user */
    if(Privsep_SetUser(uid) < 0)
        ErrorExit(SETUID_ERROR,ARGV0,user);


    debug1(PRIVSEP_MSG,ARGV0,dir,user);



    /* Signal manipulation */
    StartSIG(ARGV0);



    /* Creating PID files */
    if(CreatePID(ARGV0, getpid()) < 0)
        ErrorExit(PID_ERROR,ARGV0);


    /* Start up message */
    verbose(STARTUP_MSG, ARGV0, (int)getpid());

    /* the real stuff now */
    os_ReportdStart(&r_filter);
    exit(0);
}


/* EOF */
