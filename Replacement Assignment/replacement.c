/*
  Name: Michaela Hay
  Student ID: 1001649623
  Name: Katherine Baumann
  Student ID: 1001558704
  Replacement Assignment
  Description: Count the amount of page faults for a Reference String using
               FIFO, LRU, MFU, and Optimal page sorting algorithms.
*/

// The MIT License (MIT)
//
// Copyright (c) 2020 Trevor Bakker
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_LINE 1024
#define MAX_ARRAY 255

int FIFO_pr(int numArray[], int size, int totalPageNumbers);
int LRU_pr(int numArray[], int size, int totalPageNumbers);
int MFU_pr(int numArray[], int size, int totalPageNumbers);
int optimal_pr(int numArray[], int size, int totalPageNumbers);

/*
 * Function that implements the FIFO page replacement algo
 * 1st param, the reference string
 * 2nd param, size of the working set
 * 3rd param, total size of the reference string
 * returns, number of page faults
 */
int FIFO_pr(int numArray[], int size, int totalPageNumbers)
{
  //incremental counters and indexing trackers
  int i=0,j=0;
  //total number of page faults
  int page_faults=0;
  //boolean var to check if page requested is in the working set
  //0=not found, 1=found
  int num_found=0;
  //var to save the page meant to be replaced
  int to_replace;
  //var to save the page doing the replacing
  int replacer;
  //working set array
  int working_set[size];
  //FIFO queue to track pages
  int FIFO_queue[size];

  //Initializing the working set
  for(i=0;i<size;i++)
  {
    working_set[i]=numArray[i];
    FIFO_queue[i]=numArray[i];
    page_faults++;
  }
  //looping through the rest of the reference string
  for(i=size;i<totalPageNumbers;i++)
  {
    for(j=0;j<size;j++)
    {
      if(working_set[j]==numArray[i])
      {
        num_found=1;
      }
    }

    if(num_found==0)//if page fault
    {
      page_faults++;
      to_replace=FIFO_queue[0];
      /*request page that was not found in working set*/
      for(j=0;j<size;j++)
      {
        if(to_replace==working_set[j])
        {
          working_set[j]=numArray[i];
        }
      }
      //looping through FIFO_queue to dequeue the first page
      //and enqueue the page just added to working set
      for(j=0;j<size;j++)
      {
        if(j==(size-1))
        {
          FIFO_queue[j]=numArray[i];
        }
        else
        {
          FIFO_queue[j]=FIFO_queue[j+1];
        }
      }
    }
    //setting num_found back to 0
    num_found=0;
  }
  return page_faults;
}

/*
 * Function that implements the LRU page replacement algo
 * 1st param, the reference string
 * 2nd param, size of the working set
 * 3rd param, total size of the reference string
 * returns, number of page faults
 */
int LRU_pr(int numArray[], int size, int totalPageNumbers)
{
  //incremental counters and indexing trackers
  int i=0,j=0, array_index=0, spot=0;
  //total number of page faults
  int page_faults=0;
  //boolean var to check if page requested is in the working set
  //0=not found, 1=found
  int num_found=0;
  //var to save the page meant to be replaced
  int to_replace;
  //working set array
  int working_set[size];
  //var to run while loop
  int replacing=1;
  //counts down as we narrow down the possible replacement pages
  int counter;
  //copy of workingset
  int workingset_copy[size];

   //Initializing the working set
  for(i=0;i<size;i++)
  {
    working_set[i]=numArray[i];
    page_faults++;
  }
  //looping through the rest of the reference string
  for(i=size;i<totalPageNumbers;i++)
  {
    for(j=0;j<size;j++)
    {
      if(working_set[j]==numArray[i])
      {
        num_found=1;
      }
    }

    if(num_found==0)//if page fault
    {
      page_faults++;
      counter=size;
      array_index=i;

      //making copy of working_set
      for(j=0;j<size;j++)
      {
        workingset_copy[j]=working_set[j];
      }

      //finds the last recently used page to eliminate from working set
      while(replacing)
      {
        spot=-1;
        //going backwards in numArray
        array_index--;
        for(j=0;j<counter;j++)
        {
          //checking to find if specific is in working_set
          //implemented to deal with duplicate pages in a row
          if(workingset_copy[j]==numArray[array_index])
          {
            spot=j;
          }
        }
        //if the number was found in working_set
        if(spot!=-1)
        {
          counter--;
          //delete the number from workingset_copy
          for(j=spot;j<counter;j++)
          {
            workingset_copy[j]=workingset_copy[j+1];
          }
        }
        //when workingset_copy is down to one page, stop
        if(counter==1)
        {
          to_replace=workingset_copy[0];
          replacing=0;
        }
      }
      /*request page that was not found in working set*/
      for(j=0;j<size;j++)
      {
        //printf("%d: %d\n",j+1, working_set[j]);
        if(to_replace==working_set[j])
        {
          working_set[j]=numArray[i];
        }
      }
    }
    //setting num_found back to 0 and replacing back to 1
    num_found=0;
    replacing=1;
  }
  return page_faults;
}

int MFU_pr(int numArray[], int size, int totalPageNumbers)
{
  int page_faults = 0, working_set[size], num_found = 0;
  int ref_index = 0, working_index = 0, k = 0, m = 0, i = 0;
  /* Counters for 0-9 -- 
     incremented when numArray is searched for MFU */
                        //0, 1, 2, 3, 4, 5, 6, 7, 8, 9
  int counterArray[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  /* Fill the working_set to size with Refrence String and increment faults */
  for (ref_index = 0; ref_index < size; ref_index++)
  {
    working_set[ref_index] = numArray[ref_index];
    page_faults++;
  }

  /* Commence the MFU Algorithm */
  /* ref_index keeps track of Ref String index */
  for (ref_index = size; ref_index < totalPageNumbers; ref_index++)
  {
    num_found = 0;

    /* working_index keeps track of table's index */
    for (working_index = 0; working_index < size; working_index++)
    {
      /* If number is alrady within the table, don't increment page_faults */
      if (working_set[working_index] == numArray[ref_index])
      {
        num_found = 1;
      }
    }

    /* If the number has not been found in the table, find index to replace */
    if (num_found == 0)
    {

      /* Go through working_set and for each number within the table,
         increment how many times it appears in the Reference String (numArray) */
      /* i keeps track of position in the Table */
      for (i = 0; i < size; i++)
      {
        /* m keeps track of position in the Ref String*/
        for (m = 0; m < ref_index; m++)
        {
          /* If number in Table is found in the Reference String, iterate counter */
          if (working_set[i] == numArray[m])
          {
            counterArray[working_set[i]]++;
          }
        }
      }

      /* See which numbers in the Table appear the most in the Reference String and replace 
         the highest one. If there are multiple vying for the highest, choose the first one.
         If all are the highest, choose the first one. Increment page_faults.*/
      /* Do not use max as the value to change in the Table. 
         This is amount of OCCURENCES not the actual number.
         Use the INDEX (k) instead! */
      int max = counterArray[working_set[0]];
      int number_to_replace = working_set[0];
      for (k = 1; k < size; k++)
      {
        if (counterArray[working_set[k]] == max)
        {
          number_to_replace = number_to_replace;
        }
        if (counterArray[working_set[k]] > max)
        {
          max = counterArray[working_set[k]];
          number_to_replace = working_set[k];
        }
      }

      /* Identify the location of the number that appears the most within
         the Table and replace it with the one in the Reference String */
      for (k = 0; k < size; k++)
      {
        if (working_set[k] == number_to_replace)
        {
          working_set[k] = numArray[ref_index];
        }
      }
      page_faults++;
    }
    
    /* Set all values for counterArray back to 0 for next number in the Ref String */
    for (k = 0; k < 10; k++)
    {
      counterArray[k] = 0;
    }
  }
  return page_faults;
}

int optimal_pr(int numArray[], int size, int totalPageNumbers)
{
  //incremental counters and indexing trackers
  int i=0,j=0, array_index=0, spot=0;
  //total number of page faults
  int page_faults=0;
  //boolean var to check if page requested is in the working set
  //0=not found, 1=found
  int num_found=0;
  //var to save the page meant to be replaced
  int to_replace;
  //working set array
  int working_set[size];
  //var to run while loop
  int replacing=1;
  //counts down as we narrow down the possible replacement pages
  int counter;
  //copy of workingset
  int workingset_copy[size];

   //Initializing the working set
  for(i=0;i<size;i++)
  {
    working_set[i]=numArray[i];
    page_faults++;
  }
  //looping through the rest of the reference string
  for(i=size;i<totalPageNumbers;i++)
  {
    for(j=0;j<size;j++)
    {
      if(working_set[j]==numArray[i])
      {
        num_found=1;
      }
    }

    if(num_found==0)//if page fault
    {
      page_faults++;
      counter=size;
      array_index=i;

      //making copy of working_set
      for(j=0;j<size;j++)
      {
        workingset_copy[j]=working_set[j];
      }

      //finds the last recently used page to eliminate from working set
      while(replacing)
      {
        spot=-1;
        //going backwards in numArray
        array_index++;
        for(j=0;j<counter;j++)
        {
          //checking to find if specific is in working_set
          //implemented to deal with duplicate pages in a row
          if(workingset_copy[j]==numArray[array_index])
          {
            spot=j;
          }
          else if(numArray[array_index]==-1)
          {
            spot=0;
          }
        }
        //if the number was found in working_set
        if(spot!=-1)
        {
          counter--;
          //delete the number from workingset_copy
          for(j=spot;j<counter;j++)
          {
            workingset_copy[j]=workingset_copy[j+1];
          }
        }
        //when workingset_copy is down to one page, stop
        if(counter==1)
        {
          to_replace=workingset_copy[0];
          replacing=0;
        }
      }
      /*request page that was not found in working set*/
      for(j=0;j<size;j++)
      {
        if(to_replace==working_set[j])
        {
          working_set[j]=numArray[i];
        }
      }
    }
    //setting num_found back to 0 and replacing back to 1
    num_found=0;
    replacing=1;
  }
  return page_faults;
}

/*
 * Read in input file from command line and parses into tokens
 * then passes the array of "pages" to each page replacement algo
 */
int main( int argc, char * argv[] ) 
{
  char * line = NULL;
  size_t line_length = MAX_LINE;
  char * filename;
  /* Total amount of values in the Reference String */
  int totalPageNumbers = 0;
  /* Array that holds the Reference String (and potentially Garbage Values)
     due to size being 255 */
  int numArray[MAX_ARRAY];
  /* Moved working_set_size into main scope */
  int working_set_size = 0;
  /* Incremental Integer */
  int j = 0, i = 0;
  /* number of page faults for each page replacement algo*/
  int LRU_pf=0, FIFO_pf=0, MFU_pf=0, optimal_pf=0;

  FILE * file;

  if( argc < 2 )
  {
    printf("Error: You must provide a datafile as an argument.\n");
    printf("Example: ./fp datafile.txt\n");
    exit( EXIT_FAILURE );
  }

  filename = ( char * ) malloc( strlen( argv[1]));
  line     = ( char * ) malloc( MAX_LINE );

  memset( filename, 0, strlen( argv[1] +1 ) );
  strcpy( filename, argv[1] );

  printf("Opening file %s\n", filename );
  file = fopen( filename , "r");

  //filling whole numArray with -1
  for(j=0;j<MAX_ARRAY;j++)
  {
    numArray[i]=-1;
  }

  if ( file ) 
  {
    while ( fgets( line, line_length, file ) )
    {
      char * token;

      token = strtok( line, " ");
      /* Page table amount stored in working_set_size */
      working_set_size = atoi( token );

      while( token != NULL )
      {
        token = strtok( NULL, " " );
        if( token != NULL )
        {
          /* Store Reference Strings into numArray! */
          totalPageNumbers++;
          numArray[i] = atoi ( token );
        }
        i++;
      }
      /* Print Reference String to User */
      printf("\n");

      FIFO_pf=FIFO_pr(numArray, working_set_size, totalPageNumbers);
      printf("Page faults of FIFO: %6d\n", FIFO_pf);

      LRU_pf=LRU_pr(numArray, working_set_size, totalPageNumbers);
      printf("Page faults of LRU: %7d\n", LRU_pf);

      MFU_pf=MFU_pr(numArray, working_set_size, totalPageNumbers);
      printf("Page faults of MFU: %7d\n", MFU_pf);

      optimal_pf=optimal_pr(numArray, working_set_size, totalPageNumbers);
      printf("Page faults of Optimal: %3d\n", optimal_pf);

      //writing over the array again with -1
      for(j=0;j<totalPageNumbers;j++)
      {
        numArray[j]=-1;
      }
      //resetting the indexing vars
      totalPageNumbers=0;
      i=0;
    }
    free( line );
    fclose(file);
  }
  return 0;
}

