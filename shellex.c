/*
 * Ian Stephenson   - 44419093
 * Pok On Cheng     - 75147306
 * Sung Mo Koo      - 51338217
 * Cassie Liu       - 52504836
 */

/* $begin shellmain */
#include "csapp.h"
#include "memlib.h"
#include "mm.h"
#define MAXARGS   128

static int method = 0;
static char *mem_heap;

/* function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
void builtin_command(char **argv);
int validate_input(char *input);

int main() {
    char cmdline[MAXLINE]; /* Command line */

    /* Initialize the memory system and memory manager */
    mem_init();
    mm_init();
    
    while (1) {
        /* Read */
        printf("> ");
        Fgets(cmdline, MAXLINE, stdin);
        if (feof(stdin)) {
            exit(0);
        }

        /* Evaluate */
        eval(cmdline);
    } 
}
/* $end shellmain */
  
/* $begin eval */
/* eval - Evaluate a command line */
void eval(char *cmdline) {
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    
    strcpy(buf, cmdline);
    parseline(buf, argv);
    if (argv[0] == NULL) {
        return;   /* Ignore empty lines */
    }
    
    builtin_command(argv);
    return;
}
/* $end eval */

/* If first arg is a builtin command, run it and return true */
void builtin_command(char **argv) {
    if (!strcmp(argv[0], "quit")) { /* quit command */
        exit(0);
    }
    /* allocate command */
    else if (!strcmp(argv[0], "allocate")) {
        /*
         * Allocating using firstfit or bestfit is automatically handled by a macro flag NEXT_FIT. If NEXT_FIT is defined
         * then the allocation search algorithm will be bestfit, and if NEXT_FIT is not defined then firstfit will be
         * used. By default firstfit is used.
         */

        /* Need to test if the following argument is an integer; else it should fail */
        if(validate_input(argv[1]) < 0) {
            printf("\"%s\": Invalid argument\n", argv[1]);
            return;
        }
        else if(atoi(argv[1]) < 8) {
            printf("Must allocate more than 8B. Header and footer require 8B total. Nothing allocated.\n");
        }
        else {
			/* Convert string command line argument to unsigned integer */
			size_t size_to_allocate = (size_t) atoi(argv[1]);
            mm_malloc(size_to_allocate);
        }

		/* Print the block number associated with the just-allocated block */
		// printf("Just-allocated block number goes here\n");
    }
    /* free command */
    else if (!strcmp(argv[0], "free")) {
        if (validate_input(argv[1]) < 0) {
            printf("\"%s\": Invalid block number\n", argv[1]);
            return;
        }
        else {
			int blockNumber = atoi(argv[1]);
			char *bp = getBlockArrayElement(blockNumber);
            mm_freebufferinblock(bp);
            mm_free(bp);
        }
    }
    /* blocklist command */
    else if (!strcmp(argv[0], "blocklist")) {
        mm_printblocklist();
    }
    /* writeheap command */
    else if (!strcmp(argv[0], "writeheap")) {
        if(validate_input(argv[1]) == 0 &&
           (int) *argv[1] > 0 &&
           validate_input(argv[3]) == 0 &&
           (unsigned long) ((int)atoi(argv[3]) * 2) <= mm_getpayloadsize((int)atoi(argv[1]))) {
            
            /* Write character argv[2] number of times to payload */
            mm_writeheap((int)atoi(argv[1]), *argv[2], (int)atoi(argv[3]));
        }
        else if ((unsigned long) ((int)atoi(argv[3]) * 2) > mm_getpayloadsize((int)atoi(argv[1]))) {
            printf("Payload size for specified block is not big enough for command specified\n");
        }
        else {
            printf("Invalid arguments specified\n");
        }
//        
//        
//        if (validate_input(argv[1]) < 0 && validate_input(argv[3]) >= 0) {
//            printf("\"%s\": Invalid block number\n", argv[1]);
//            return;
//        }
//        else if (validate_input(argv[1]) >= 0 && validate_input(argv[3]) < 0) {
//            printf("\"%s\": Invalid number of copies\n", argv[3]);
//            return;
//        }
//        else if (validate_input(argv[1]) < 0 && validate_input(argv[3]) < 0) {
//            printf("\"%s\": Invalid block number\n", argv[1]);
//            printf("\"%s\": Invalid number of copies\n", argv[3]);
//        }
//        else {
//            unsigned long total = getpay
//        }
    }
    /* printheap command */
    else if (!strcmp(argv[0], "printheap")) {
        if (validate_input(argv[1]) < 0 && validate_input(argv[2]) >= 0) {
            printf("\"%s\": Invalid block number\n", argv[1]);
            return;
        }
        else if (validate_input(argv[1]) >= 0 && validate_input(argv[2]) < 0) {
            printf("\"%s\": Invalid number of bytes\n", argv[2]);
            return;
        }
        else if (validate_input(argv[1]) < 0 && validate_input(argv[2]) < 0) {
            printf("\"%s\": Invalid block number\n", argv[1]);
            printf("\"%s\": Invalid number of bytes\n", argv[2]);
            return;
        }
        else {
            mm_printheap((int)atoi(argv[1]), (int)atoi(argv[2]));
        }
    }
    /* bestfit command */
    else if (!strcmp(argv[0], "bestfit")) {
		/*
		 * The flag for using bestfit or firstfit was already written into the code we had. It would test to see if the
		 * NEXT_FIT macro was defined whenever it needed to user firstfit or bestfit. If NEXT_FIT is defined, then the
		 * allocation method is bestfit; if the macro is not defined then by default it uses first fit.
		 *
		 * So if the user wants to enable bestfit, check to see if NEXT_FIT is defined. If NEXT_FIT is defined, then
		 * do nothing-- the flag is already set and there is nothing to do. However, if NEXT_FIT is not defined, then
		 * define the macro and return.
		 * define it.
		 */
		#ifndef NEXT_FIT
			#define NEXT_FIT
		#endif
        return;
    }
    /* firstfit command */
    else if (!strcmp(argv[0], "firstfit")) {
		/*
		 * The flag for using bestfit or firstfit was already written into the code we had. It would test to see if the
		 * NEXT_FIT macro was defined whenever it needed to user firstfit or bestfit. If NEXT_FIT is defined, then the
		 * allocation method is bestfit; if the macro is not defined then by default it uses first fit.
		 *
		 * So if the user wants to enable firstfit (and thus disable bestfit), check to see if NEXT_FIT is defined.
		 * If NEXT_FIT is defined, then undefine NEXT_FIT-- the flag is set and it needs to be disabled. However, if
		 * NEXT_FIT is not defined, then do nothing-- the flag should be disabled and it already is, so just return.
		 */
        #ifdef NEXT_FIT
			#undef NEXT_FIT
		#endif
        return;
    }
    /* Not a builtin command */
    else {
        printf("%s: Command not found!\n", argv[0]);
    }
}
/* $end builtin_command */

/* $begin parseline */
/* parseline - Parse the command line and build the argv array */
int parseline(char *buf, char **argv)
{
	char *delim;         /* Points to first space delimiter */
	int argc;            /* Number of args */
	int bg;              /* Background job? */

	buf[strlen(buf)-1] = ' ';  /* Replace trailing '\n' with space */

	while (*buf && (*buf == ' ')) {
		buf++;           /* Ignore leading spaces */
	}

	/* Build the argv list */
	argc = 0;
	while ((delim = strchr(buf, ' '))) {
		argv[argc++] = buf;
		*delim = '\0';
		buf = delim + 1;
		while (*buf && (*buf == ' ')) {
			buf++;       /* Ignore spaces */
		}
	}
	argv[argc] = NULL;

	if (argc == 0) {
		return 1;       /* Ignore blank line */
	}

	/* Should the job run in the background? */
	if ((bg = (*argv[argc-1] == '&')) != 0) {
		argv[--argc] = NULL;
	}
	return bg;
}
/* $end parseline */

/* $begin validate_input */
/* validate_input - Validates the second command line argument to be an integer */
int validate_input(char *input) {
    /*
     * Loop through chars. Test if digit. If one is not digit return -1.
     * That means all are digits, and therefore return 0.
     */
    unsigned long length = strlen(input);
    int i;
	for (i = 0; i < length; i++){
        if(!isdigit(input[i])) {
            return -1;
        }
    }
    return 0;
}