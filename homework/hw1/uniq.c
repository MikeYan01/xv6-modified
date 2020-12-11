#include "types.h"
#include "user.h"

#define bool short
#define true 1
#define false 0
#define MAX_LINE_LEN 2048
#define READ_BUFF_SIZE 1024
#define NULL 0

typedef enum {
    ERR_C_AND_D = 100,      // arguments -c & -d
    ERR_WRONG_ARG = 101,    // argument other than c/d/i
    ERR_NO_FILE = 102,      // no such file
    ERR_READ_FILE = 103,    // read file error
    ERR_POST_ARG = 104      // argument after input file
} ERR_Code;

char currLine[MAX_LINE_LEN];
char memoLine[MAX_LINE_LEN];

void errorProcess (ERR_Code err) {
    char *output = NULL;
    switch (err) {
        case ERR_C_AND_D:
            output = "uniq: -c and -d cannot appear at the same time.";
            break;
        case ERR_WRONG_ARG:
            output = "uniq: Only three kinds of arguments allowed: -c | -d | -i.";
            break;
        case ERR_NO_FILE:
            output = "uniq: Cannot find such file, please check file name and directory.";
            break;
        case ERR_READ_FILE:
            output = "uniq: An error occurred while reading file.";
            break;
        case ERR_POST_ARG:
            output = "uniq: Command line arguments should be ahead of file name";
        default:
            break;
    }
    printf(2, "%s\n", output);
    printf(2, "usage: uniq [-c | -d] [-i] [fileName]\n");
    exit();
}

bool linecmp(bool argI) {
    int ptr = -1;

    while (currLine[++ptr] != '\0') {
        char currChar = currLine[ptr];
        char memoChar = memoLine[ptr];

        // argument -i: ignore differences in case when comparing
        if (!argI) { // case sensitive
            if (currChar != memoChar) 
                return false;
        } else { // ignore case, both coverted to lower case and compare
            if (currChar >= 'A' && currChar <= 'Z')
                currChar += 32;
            if (memoChar >= 'A' && memoChar <= 'Z')
                memoChar += 32;
            if (currChar != memoChar)
                return false;
        }
    }
    return true;
}

void printDelay(int currLineCount, bool argC, bool argD) {
    if (argC)
        printf(1, "\t%d %s", currLineCount, memoLine);
    else if (!argD || currLineCount > 1) 
        printf(1, "%s", memoLine);
}

void printInstant() {
    printf(1, "%s", currLine);
}

void uniqDelay(int fd, bool arguList[]) {
    int i; // for-loop
    int len; // valid buffer length
    int ptr = 0; // traverse currLine
    int lastLineLen = 0; // last line length
    int currLineCount = 0; // current same line count

    bool isFirstLine = true; // whether current line is the first line

    char readBuff[READ_BUFF_SIZE];

    memset(memoLine, '\0', sizeof(memoLine));
	memset(currLine, '\0', sizeof(currLine));
    
    // keep reading until EOF or error
    while (true) {
        len = read(fd, readBuff, sizeof(readBuff));
        if (len <= 0) break;

        bool partialRead = true; 

        for (i = 0; i < len; i++) {
            currLine[ptr] = readBuff[i];

            // end of current line and process
            if (readBuff[i] == '\n') {
                // current line's length is eqaul to read buffer's length
                if (ptr == i) partialRead = false;

                // if same with last line, update count and reset currLine memory
                if (ptr == lastLineLen && linecmp(arguList[2]))
                    currLineCount++;

                // not same line, decide whether to output, and store currLine to memoLine
                else {
                    if (isFirstLine) isFirstLine = false;
                    else
                        printDelay(currLineCount, arguList[0], arguList[1]);

                    currLineCount = 1; // a new line begins
                    strcpy(memoLine, currLine); 
                }
                if ((fd != 0 || partialRead == true) && i == len - 1) printDelay(currLineCount, arguList[0], arguList[1]);
                memset(currLine, '\0', sizeof(currLine));

                // update last line length and clear current line pointer
                lastLineLen = ptr;
                ptr = 0;
            } 
            else ptr++; // continue to add characters of current line
        }
    }

    // break because of error
    if (len < 0)
        errorProcess(ERR_READ_FILE);
}

void uniqInstant(int fd, bool argI) {
    int i; // for-loop
    int len; // valid buffer length
    int ptr = 0; // traverse currLine
    int lastLineLen = 0; // last line length

    bool isFirstLine = true; // whether current line is the first line

    char readBuff[READ_BUFF_SIZE];

    // keep reading until EOF or error
    while (true) {
        len = read(fd, readBuff, sizeof(readBuff));
        if (len <= 0) 
            break;

        for (i = 0; i < len; i++) {
            currLine[ptr] = readBuff[i];

            if (readBuff[i] == '\n') { // end of current line and process
                if (isFirstLine) {
                    printInstant();
                    isFirstLine = false;
                    strcpy(memoLine, currLine);
                } else if (ptr == lastLineLen && linecmp(argI)) // if same with last line, update count and reset currLine memory
                    memset(currLine, '\0', sizeof(currLine));
                else { // not same line, decide whether to output, and store currLine to memoLine
                    printInstant();
                    strcpy(memoLine, currLine);
                }

                // update last line length and clear current line pointer
                lastLineLen = ptr;
                ptr = 0;
            } else // continue to add characters of current line
                ptr++;
        }
    }

    // break because of error
    if (len < 0)
        errorProcess(ERR_READ_FILE);
}

int main(int argc, char *argv[]) {
    bool arguList[3]; // [0] -> -c, [1] -> -d, [2] -> -i
    memset(arguList, false, sizeof(arguList));
    memset(memoLine, '\0', sizeof(memoLine));
	memset(currLine, '\0', sizeof(currLine));

    int fd = 0; // file descriptor
    int i = 0; // for-loop
    char* fileName = NULL; // input file name

    // Process argv[]
    for (i = 1; i < argc; i++)
        if (*argv[i] == '-') { // Command Line Argument: argument starts with '-'

            // Error: file name ahead of command line argument
            if (fileName != NULL)
                errorProcess(ERR_POST_ARG);

            // Support multi arguments under one '-' like '-ci' or '-di'
            char* ptr = argv[i];
            char* end = ptr + strlen(argv[i]);
            ptr++;
            
            // Go through the "-XXX" argument format to extract each argument
            while (ptr < end)
                switch (*ptr) {
                    case 'c':
                        arguList[0] = true;
                        ptr++;
                        continue;
                    case 'd':
                        arguList[1] = true;
                        ptr++;
                        continue;
                    case 'i':
                        arguList[2] = true;
                        ptr++;
                        continue;
                    default:
                        errorProcess(ERR_WRONG_ARG);
                        break;
                }
        } else
            fileName = argv[i]; // Input file: argument without leading '-'

    // Get input source
    if (fileName == NULL) // Standard input (pipe or keyboard) if no filename provided
        fd = 0;
    else // Read from file (try to open file)
        fd = open(fileName, 0);

    // Error: both -c & -d exist
    if (arguList[0] && arguList[1])
        errorProcess(ERR_C_AND_D);

    // Error: open file error
    if (fd < 0)
        errorProcess(ERR_NO_FILE);

    // uniq based on command line args
    if (arguList[0] || arguList[1]) // delay output if -c or -d
        uniqDelay(fd, arguList);
    else // instant output without -c & -d
        uniqInstant(fd, arguList[2]);

    close(fd);
    exit();
}