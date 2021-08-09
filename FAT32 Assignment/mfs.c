/* Name: Michaela Hay
     ID: 1001649623
   Name: Katherine Baumann
     ID: 1001558704
     
*/

// The MIT License (MIT)
// 
// Copyright (c) 2016, 2017 Trevor Bakker s
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports five arguments

#define MAX_NAME 11             //names of files can be max 11

#define MAX_DIR_SIZE 16         //directories will have a maximum of 16 files

#define MAX_FILE_EXT 3          //File extensions are 3 chars max

struct __attribute__((__packed__)) DirectoryEntry
{
  char DIR_Name[MAX_NAME];
  uint8_t Dir_Attr;
  uint8_t Unused1[8];
  uint16_t DIR_FirstClusterHigh;
  uint8_t Unused[4];
  uint16_t Dir_FirstClusterLow;
  uint32_t DIR_FileSize;
};

struct DirectoryEntry dir[MAX_DIR_SIZE];
struct DirectoryEntry rootDir[MAX_DIR_SIZE]; // keeps track of root directory
int is_open; //keeps track of if a file is open or not
FILE *fp; //file pointer
FILE *fout; //file out
char filename[100]; //holds filename
char converted[MAX_NAME]; //holds converted filename
char converted_folder[MAX_NAME]; //holds converted foldername
int root_cluster; //amount of bits to root cluster

static uint16_t bytespersec;
static uint8_t secperclus;
static uint16_t rsvdseccnt;
static uint8_t numfats;
static uint32_t fatsz32;

int decider(char **token);
int open_cmd(char * filename);
int close_cmd(char * filename);
void info_cmd();
void cmd_ls();
void cmd_cd(char * userString);
void calculate_clusters();
char* convert_filename(char * file_to_convert);
char* convert_foldername(char * folder_to_convert,int spaces);
void fill_dir();
void read_cmd(char * file_to_read, char * secondarg, char * thirdarg);
int LBAToOffset (int32_t sector);
int16_t NextLB(uint32_t sector);
void stat_cmd(char * file_to_find);
void get_cmd(char * file_to_get);

/*
 *Function:LBAToOffset
 *Parameters: The current sector mumber that points to a block of data
 * Returns: The value of the address for that block of data
 * Description: Finds the starting address of a block of data given the sector number
 * corresponding to that data block.
 */
int LBAToOffset(int32_t sector)
{
  return ((sector -2)*bytespersec)+(bytespersec*rsvdseccnt)+(numfats*fatsz32*bytespersec);
}

/*
 *Function:NextLB
 *Parameters: Given a logical block address, look up into the first FAT and
 *return the logical block address of the block in the file.
 *If there is no further blocks then return -1;
 */
int16_t NextLB(uint32_t sector)
{
  uint32_t FATAddress=(bytespersec*rsvdseccnt)+(sector*4);
  int16_t val;
  fseek(fp,FATAddress,SEEK_SET);
  fread(&val, 2, 1, fp);
  return val;
}

/*
 *Function:convert_foldername
 * Purpose: To convert foldername to uppercase
 * Parameters: char array for file to convert, length of foldername
 * Returns: converted filename
 */
char* convert_foldername(char* folder_to_convert,int spaces)
{
  int i=0;
  int spaces_to_add=MAX_NAME-(spaces-1);
  memset(converted_folder,'\0',MAX_NAME);
  strcpy(converted_folder,folder_to_convert);
  while(converted_folder[i]!='\0')
  {
    converted_folder[i]=toupper(converted_folder[i]);
    i++;
  }
  return converted_folder;
}

/*
 *Function:convert_filename
 * Purpose: To convert filename in format "foo.txt" into FOO    TXT
 * Parameters: char array for file to convert
 * Returns: converted filename
 */
char* convert_filename(char * file_to_convert)
{
    memset(converted,'\0',MAX_NAME);
  int i=0,j=0,k=0,amount_spaces=(MAX_NAME-3),amount_sub=0;
  char input[MAX_NAME];
  char ext[MAX_FILE_EXT];
  memset(input,'\0',MAX_NAME);
  memset(ext,'\0',MAX_FILE_EXT);
  strcpy(input,file_to_convert);
  strcat(input,".");
  char* token=strtok(input,".");
  strcpy(converted,token);
  token=strtok(NULL,".");
  strcpy(ext,token);
  if(strncmp(file_to_convert,"folder",6)==0)
  {
    strncpy(converted,convert_foldername(input,sizeof(token)),MAX_NAME);
    return converted;
  }
  while(converted[i]!='\0')
  {
    converted[i]=toupper(converted[i]);
    i++;
  }
  amount_spaces=amount_spaces-i;
  for(j=0;j<amount_spaces;j++)
  {
    converted[i]=' ';
    i++;
  }
  while(ext[k]!='\0')
  {
    ext[k]=toupper(ext[k]);
    k++;
  }
  strcat(converted,ext);
}

/*
 *Extracting information from volumne ID and reserved sectors
 */
void calculate_clusters()
{
  /*Bytes Per Second*/
  fseek(fp,11,SEEK_SET);
  fread(&bytespersec,2,1,fp);

  /* See Per Cluster */
  fseek(fp,13,SEEK_SET);
  fread(&secperclus,1,1,fp);

  /* RSVD See Cnt */
  fseek(fp,14,SEEK_SET);
  fread(&rsvdseccnt,2,1,fp);

  /* NumFATS */
  fseek(fp,MAX_DIR_SIZE,SEEK_SET);
  fread(&numfats,1,1,fp);

  /* FATSz32 */
  fseek(fp,36,SEEK_SET);
  fread(&fatsz32,4,1,fp);
}

/*
 *Filling dir array of all directories in .img file system
 */
void fill_dir()
{
  int i = 0;
  root_cluster = (numfats * fatsz32 * bytespersec) + (rsvdseccnt * bytespersec);
  fseek(fp, root_cluster, SEEK_SET);
  fread(&dir[0], MAX_DIR_SIZE, sizeof(struct DirectoryEntry), fp);

  for (i = 0; i < MAX_DIR_SIZE; i++)
  {
    rootDir[i] = dir[i];
  }
}

/* Function takes in the name of the directory the user wants to access,
   calculates where it is located, and goes to that location. */
void cmd_cd (char * userString)
{
  char * token;
  // Can have 100 total directories referenced 
  // and each directory name can be 100 characters long
  char directoryNames[100][100];
  int directoryIndex = 0;
  int i = 0;
  int j = 0;
  int m = 0;
  int offset = 0;
  token = strtok(userString, "/");
  /* Save the directory names into directoryNames */
  while (token)
  {
    strcpy(directoryNames[directoryIndex], token);
    token = strtok(NULL, "/");
    directoryIndex++;
  }

  /* Find matches for the user's desired directories to cd into
     and then perform fseek and fread on them */
  for (j = 0; j < directoryIndex; j++)
  {
    /* Look inside each dir from the struct to find match of names*/
    for (i = 0; i < MAX_DIR_SIZE; i++)
    {
      char * tempDirName;
      /* Tokenize the dir's name based on whitespace:
          "FOLDERA " -> "FOLDERA" */
      tempDirName = strtok(dir[i].DIR_Name, " ");

      /* Make sure tempDirName is not null so
         strcmp() won't throw an error */
      if (tempDirName)
      {
        /* If a match is found within the directory, find cluster */
        if (strcmp(directoryNames[j], tempDirName) == 0)
        {
          if (dir[i].Dir_FirstClusterLow == 0)
          { 
            /* Copy root directory into directory if ".." */
            for (m = 0; m < MAX_DIR_SIZE; m++)
            {
              dir[m] = rootDir[m];
            }
            break;
          }
          else
          {
            offset = LBAToOffset(dir[i].Dir_FirstClusterLow);
            fseek(fp, offset, SEEK_SET);
            fread(&dir[0], MAX_DIR_SIZE, sizeof(struct DirectoryEntry), fp);
          }
          i = 0;
        }
      }
    }
  }
}

/* Function calculates location of root cluster,
   then from there reads the information into the dir array,
   and then prints out the subdirectories and filenames. */
void cmd_ls()
{
  int i;
  for (i = 0; i < MAX_DIR_SIZE; i++)
  {
    if (dir[i].Dir_Attr == 0x01 || dir[i].Dir_Attr == 0x10 || dir[i].Dir_Attr == 0x20)
    {
      printf("%.12s\n", dir[i].DIR_Name);
    }
  }

}

/* opens the file specified by use
 * 1st param, takes filename pointer
 * returns 1 if fileopen was successful, returns 0 if failed
 */
int open_cmd(char * filename)
{
  char token [1000];
  fp=fopen(filename, "r+");
  while(fgets(token,0,fp))
  {
    printf("%s\n",token);
  }
  if(fp==NULL)
  {
    return 0;
  }
  else
  {
    return 1;
  }
}

/* closes the file currently open
 * 1st param, takes filename pointer
 * returns 1 if fileclose was successful, returns 0 if failed
 */
int close_cmd(char * filename)
{
  int check;
  check=fclose(fp);
  if(check==0)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

/*
 *Printing out info for "info" command
 */
void info()
{
  printf("BPB_BytesPerSec : %d\n",bytespersec);
  printf("BPB_BytesPerSec : %x\n",bytespersec);

  printf("BPB_SecPerClus : %d\n",secperclus);
  printf("BPB_SecPerClus : %x\n",secperclus);

  printf("BPB_RsvdSecCnt : %d\n",rsvdseccnt);
  printf("BPB_RsvdSecCnt : %x\n",rsvdseccnt);

  printf("BPB_NumFATS : %d\n",numfats);
  printf("BPB_NumFATS : %x\n",numfats);

  printf("BPB_FATSz32 : %d\n",fatsz32);
  printf("BPB_FATSz32 : %x\n",fatsz32);
}

/*
 *Function:read_cmd
 * Purpose: To read a number of bits in a file
 * Parameters: char array for file to read, char array for offset, char array for amount of bits to read
 * Returns: void
 */
void read_cmd(char * file_to_read, char * secondarg, char * thirdarg)
{
  int i=0,index=-1;
  char converted_name[MAX_NAME];
  strcpy(converted_name, convert_filename(file_to_read));
  printf("%s\n",converted_name);
  for(i=0;i<MAX_DIR_SIZE;i++)
  {
    if(strncmp(dir[i].DIR_Name,converted_name,7)==0)
    {
      index=i;
    if(strncmp(dir[i].DIR_Name,"FOLDER",6)==0)
    {
      index=-2;
    }
    }
  }
  if(index==-1)
  {
    printf("File not found!\n");
    return;
  }
  else if(index==-2)
  {
    printf("Cannot read from a folder!\n");
    return;
  }
  int offset=atoi(secondarg);
  int bytes=atoi(thirdarg);
  int size=dir[index].DIR_FileSize;
  if((size-offset)<bytes)
  {
    printf("Bytes to read is bigger than the file size!\n");
    return;
  }
  int cluster=dir[index].Dir_FirstClusterLow;
  fseek(fp,LBAToOffset(cluster)+offset,SEEK_SET);
  printf("Reading from %s....\n",file_to_read);
  for(i=0;i<bytes;i++)
  {
    unsigned char val;
    fread(&val,1,1,fp);
    printf("%x\n",val);
  }
  printf("Done reading from %s.\n",file_to_read);
  return;
}

/*
 *Function:get_cmd
 * Purpose: To grab file that is specified and put it in the working directory
 * Parameters: char array for file to get
 * Returns: void
 */
void get_cmd(char * file_to_get)
{
  int i=0,index=-1;
  char converted_name[MAX_NAME];
  strcpy(converted_name, convert_filename(file_to_get));
  for(i=0;i<MAX_DIR_SIZE;i++)
  {
    if(strncmp(dir[i].DIR_Name,converted_name,7)==0)
    {
      index=i;
    }
  }
  if(index==-1)
  {
    printf("File not found!\n");
    return;
  }
  int cluster=LBAToOffset(dir[index].Dir_FirstClusterLow);
  fseek(fp,cluster,SEEK_SET);
  int bytes=dir[index].DIR_FileSize;
  fout=fopen(file_to_get, "w+");
  for(i=0;i<bytes;i++)
  {
    unsigned char val;
    fread(&val,1,1,fp);
    fputs(&val,fout);
    if(bytes%16==0)
    {
      fputs("\n",fout);
    }
  }
  fclose(fout);
}

/*
 *Function:stat_cmd
 * Purpose: To print out attributes about a file
 * Parameters: char array for file to print stats about
 * Returns: void
 */
void stat_cmd(char * file_to_find)
{
  int i=0,index=-1;
  char converted_name[MAX_NAME];
  strcpy(converted_name, convert_filename(file_to_find));
  for(i=0;i<MAX_DIR_SIZE;i++)
  {
    if(strncmp(dir[i].DIR_Name,converted_name,7)==0)
    {
      index=i;
    }
  }
  if(index==-1)
  {
    printf("File not found!\n");
    return;
  }
  int cluster=LBAToOffset(dir[index].Dir_FirstClusterLow);
  printf("File: %s\n",file_to_find);
  printf("File Attribute: %d\n",dir[index].Dir_Attr);
  printf("First Cluster High: %d\n",dir[index].DIR_FirstClusterHigh);
  printf("First Cluster Low: %d\n",dir[index].Dir_FirstClusterLow);
  printf("File size: %d\n",dir[index].DIR_FileSize);
  printf("Starting cluster number: %d\n",cluster);
}

/* decides what to do with commands
 * 1st param, takes array of char pointers
 * returns -1 if user wants to quit, returns 0 if not
 */
int decider(char **token)
{
  int check; //keeps track if file open/close was successful

  if(token[0]==NULL)
  {
    return 0;
  }
  else if(strcmp(token[0], "quit")==0||
          strcmp(token[0], "exit")==0)//exit command
  {
    return -1;
  }
  else if(strcmp(token[0],"open")==0)//open command
  {
    if(is_open)
    {
      printf("Error: File system image already open.\n");
      return 0;
    }
    else
    {
      check=open_cmd(token[1]);
      if(check)
      {
        printf("Opening %s.\n", token[1]);
        is_open=1;
        strncpy(filename,token[1],100);
        calculate_clusters();
        fill_dir();
      }
      else
      {
        printf("Error opening %s.\n", token[1]);
      }
      return 0;
    }
  }
  else if(strcmp(token[0],"close")==0)
  {
    if(is_open)
    {
      check=close_cmd(filename);
      if(check)
      {
        is_open=0;
        printf("Closing %s.\n",filename);
      }
      else
      {
        printf("Error closing %s.\n", filename);
      }
      return 0;
    }
    else
    {
      printf("Error: File system not open.\n");
      return 0;
    }
  }
    else if(strcmp(token[0],"info")==0)
  {
    if(is_open)
    {
      info();
      return 0;
    }
    else
    {
      printf("Error: File system image must be opened first.\n");
      return 0;
    }
  }
  else if(strcmp(token[0],"stat")==0)
  {
    if(is_open)
    {
      stat_cmd(token[1]);
      return 0;
    }
    else
    {
      printf("Error: File system image must be opened first.\n");
      return 0;
    }
  }
  else if(strcmp(token[0],"get")==0)
  {
    if(is_open)
    {
      get_cmd(token[1]);
      return 0;
    }
    else
    {
      printf("Error: File system image must be opened first.\n");
      return 0;
    }
  }
  else if(strcmp(token[0],"cd")==0)
  {
    if(is_open)
    {
      if (token[1])
      {
        cmd_cd(token[1]);
      }
      else
      {
        printf("You did not list a directory.\n");
      }
      return 0;
    }
    else
    {
      printf("Error: File system image must be opened first.\n");
      return 0;
    }
  }
  else if(strcmp(token[0],"ls")==0)
  {
    if(is_open)
    {
      cmd_ls();
      return 0;
    }
    else
    {
      printf("Error: File system image must be opened first.\n");
      return 0;
    }
  }
  else if(strcmp(token[0],"read")==0)
  {
    if(is_open)
    {
      read_cmd(token[1],token[2],token[3]);
      return 0;
    }
    else
    {
      printf("Error: File system image must be opened first.\n");
      return 0;
    }
  }
  else
  {
    printf("Command not found!\n");
    return 0;
  }
}

int main()
{
  //null terminating filename
  memset(filename,'\0',100);

  int should_exit=0;

  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );

  while( 1 )
  {
    // Print out the mfs prompt
    printf ("mfs> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    int   token_count = 0;                                 
                                                           
    // Pointer to point to the token
    // parsed by strsep
    char *arg_ptr;                                         
                                                           
    char *working_str  = strdup( cmd_str );                

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;

    // Tokenize the input stringswith whitespace used as the delimiter
    while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) && 
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = '\0';
      }
        token_count++;
    }

  //passing to the decider that determines what to do with command
  should_exit=decider(token);
  if(should_exit==-1)
  {
    exit(0);
  }
    free( working_root );
  }
  return 0;
}

