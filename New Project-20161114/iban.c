/*
*t	iban [-ib] [-f <inputfile> ] <iban nr>
*	check validity of iban account
*
*	compile with : cc -lm -o iban iban.c
*	PBILLAST-20110810
*	PBILLAST-20111025
* 	added numerous vars and functions
*m	PBILLAST-20140225
*	argc starts at optind after options are read
*	added ARGS_EXPECTED
*m	PBILLAST-20140630
*	adjust argc and argv according to optind
*	
*/
/* toegevoegd voor gcc */
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <math.h>

#include <stdio.h>
#include <string.h>
#include <regex.h>
#include <stdbool.h>

#define LINESIZE 4096
#define MAX_ERROR_MSG 40
/*******************************************************************************
*   print_debug
*******************************************************************************/
int print_debug(char * formatstr,char * debugstr,int debug) {
if ( debug == 1 ) {
		fprintf(stderr,formatstr,debugstr);
		}
	}

/*******************************************************************************
*	open_file
*******************************************************************************/
FILE * open_file(char * fname ,char mode[2],int debug) {
FILE *fp;
fp = fopen(fname,mode);
if ( fp == NULL ) {
	fprintf(stderr,"error: file does not exist\n"); 
	exit(2);
	}
else
	print_debug("file opened %s\n",fname,debug);
	return fp;
}

/*******************************************************************************
* FUNCTIONS
*******************************************************************************/
void usage() {
	printf("usage: iban [-bi] [-f <inputfile>] <iban or bbic>\n");
}

char * substr(char * str, int from, int length) {
	char * s=malloc(sizeof ( char ) * 100 );
	memcpy(s,&str[from],length);
	s[length]='\0';
	return(s);
}

char * numerify(char * str) {
	char * asnum=malloc (sizeof(char) * 100);
	char buffer2[2];
	char buffer1[1];
	int i;
	int val;
	/* replace all chars (A-10,B-11,C-12,...) */
	for (i = 0; i < strlen(str); i++) {
		if ( str[i] >= 65 && str[i] <= 90 ) {
			sprintf(buffer2,"%2d",str[i]-55);
			strcat(asnum,buffer2);
			}
		else {
			sprintf(buffer1,"%c",str[i]);
			strcat(asnum,buffer1);
			}
	}
	return(asnum);
}

bool inarray(char val[2], char * arr[100], int size) {
	int i;
	char cval[2];
	for (i=0; i < size; i++) {
		strcpy(cval,arr[i]);
		printf("compare: %s\n",cval);
		if ( strcmp(arr[i],cval)==0 ) {
			printf("found: %s %s\n",arr[i],val);
			return true;
			}
		}
	return false;
	}

int modulo(char Cval[LINESIZE],int div) {
	char _Cval[LINESIZE]="";
	int val,mod;
	strcpy(_Cval,Cval);
	/* strip leading 0 */
	while ( _Cval[0]==48 ) { 
		strcpy(_Cval,substr(_Cval,1,strlen(_Cval)-1));
		}
	while ( strlen(_Cval)>5 ) {
		val=atol(substr(_Cval,0,3));
		mod=val-div;
		printf("Cval: %s mod:%d\n",_Cval,mod);
		sprintf(_Cval,"%d%s\0",mod,substr(_Cval,3,strlen(_Cval)-3));
		}
	return atol(_Cval)%div;
	}

int check_iban(char * iban,regex_t regex,int debug) {  
    int reti=0;
    int l;
    char newiban[50]="";
    char * Cmodulo;
	int Imodulo;

    char * _iban;
    _iban=strdup(iban);
    reti = regexec(&regex,_iban, 0, NULL, 0);
    print_debug("iban to check: %s\n",_iban,debug);
    if ( !reti ) {
            print_debug("Valid chars!\n",0,debug);
            /* modulo */

/*2345678901234567890123456789012345678901234567890123456789012345678901234567*/
            /* put countrycode at the end */
            l=strlen(_iban);
            strcat(newiban,substr(_iban,4,strlen(_iban)-4));
            strcat(newiban,substr(_iban,0,2));
            Cmodulo=substr(_iban,2,2);
            print_debug("iban: -> %s\n",newiban,debug);
            strcpy(_iban,numerify(newiban));
            strcat(_iban,"00");
            print_debug("newiban: %s\n",_iban,debug);

            /* cut off last 2 chars and check modulo */
            Imodulo=strtol(Cmodulo, &Cmodulo, 10);
            if ( ( 98 - modulo(_iban,97) ) == Imodulo ) {
                    print_debug("modulo ok\n","",debug);
                    return 0;
                    }
            else {
                    print_debug("modulo NOT ok\n","",debug);
                    fprintf(stderr,"%s:%d",iban,2); 
                    fprintf(stderr," iban: %s",_iban);
                    fprintf(stderr," calc:%d",98 - modulo(_iban,97));
                    fprintf(stderr," mod:%2d\n",Imodulo);
                    return 2;
                    }
            }
    else {
            print_debug(" Invalid characters\n","",debug);
            fprintf(stderr,"%s:%d\n",iban,1); 
            return 1;
        }
	}

int check_bbic(char * bbic, FILE *fp,char countries[100][3],int debug) {  
char country[3];
int i;
	strcpy(country,substr(bbic,4,2));
	if ( strlen(bbic) == 8 || strlen(bbic) == 11 ) {
		print_debug("length is ok\n","",debug);
		for ( i=0 ; i < 20 ; i++ ) {
			if( (strcmp(countries[i],country)==0) && country ) {
				print_debug("country %s found\n",countries[i],debug);
				return 0;
				}
			}
		print_debug("country NOT in list\n","",debug);
		fprintf(stderr,"%s:%d\n",bbic,4); 
		return 4;
		}
	else {
		print_debug("length not 8 or 11\n","",debug);
		fprintf(stderr,"%s:%d\n",bbic,3); 
		return 3;
		}
	}
/*******************************************************************************
* MAIN
*******************************************************************************/
FILE *fp;
int main(int argc,char ** argv) {
int c,i,reti;
int fflag=0;
char * Cval;
int debug=0;
regex_t regex;
char * fvalue=NULL;
char refcountry[2];
char line[LINESIZE];
char check='i';
char countries[100][3];
int ARGS_EXPECTED=1;

FILE * fp;

while (( c = getopt (argc, argv, "hdibf:")) != -1)
	switch (c) {
		case 'h':
			usage();
			exit(0);
		case 'd':
			debug=1;
			break;
		case 'i':
			/* iban check */
			check='i';
			break;
		case 'b':
			/* bbic check */
			check='b';
			break;
		case 'f':
			/* get input values from file */
                        fflag=1;
			fvalue=optarg  ;
                        ARGS_EXPECTED=0;
			break;
                case '?':
                        if (optopt == 'f')
                            fprintf (stderr, "Option -%c requires an argument.\n", optopt);
                        else if (isprint (optopt))
                            fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                        else
                            fprintf (stderr, "Unknown option character `\\x%x'.\n",optopt);
                        return 1;
                default:
                        abort ();                        
                        break;
	}
if ( argc < optind + ARGS_EXPECTED) { usage(); return 1; }
/* start reading at argv[optind] */
	argc -= optind;
	argv += optind;


switch (check) {
case 'i': /* iban */
	reti = regcomp(&regex, "^[a-zA-Z0-9]*$", REG_EXTENDED);
	if (reti) {
		fprintf(stderr, "Could not compile regex\n");
		exit(1);
		}
	break;
case 'b':   /* bbic */
	fp = open_file("/usr/local/lbase_custom/etc/SEPA/countrycode.lst","r",debug);
	/* get list of countries */
	i=0;
	print_debug("get list\n","",debug);
	while ( fgets (line , LINESIZE , fp ) != NULL ) {
		strcpy(countries[i++],substr(line,0,2));
		if (debug == 1 ) {
			printf("refcountry %d: %s\n",i,countries[i-1]);
			}
		}
	fclose(fp);
	break;
	}
/*2345678901234567890123456789012345678901234567890123456789012345678901234567*/
if ( fflag == 0 ) {
	Cval = argv[0];
	switch (check) {
	case 'i': /* iban */
		check_iban(Cval,regex,debug);
		break;
	case 'b': /* bbic */
		check_bbic(Cval,fp,countries,debug);
		break;
		}
	}
else {
	/* check a file full of iban numbers */
	fp = open_file(fvalue,"r",debug);
	while ( fgets (line , LINESIZE , fp ) != NULL ) {
		/*
		Cval=(strtok(line, "\n"));
		*/
		Cval=strtok(line, "\n");
		switch (check) {
			case 'i': /* iban */
				check_iban(Cval,regex,debug);
				break;
			case 'b': /* bbic */
				check_bbic(Cval,fp,countries,debug);
				break;
			}
		}
	fclose(fp);
	}
}
