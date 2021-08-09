/*
  Michaela Hay
  1001649623
  Katherine Baumann
  1001558704
*/
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#define MAX_SEATS 3 //Number of seats in the professor's office
#define professor_LIMIT 10 //Number of students the professor can help before he needs a break
#define MAX_STUDENTS 1000 //Maximum number of students in the simulation

#define CLASSA 0
#define CLASSB 1

sem_t mutex; //defining semaphore to control sychronization
pthread_mutex_t professorLock; //mutex for controlling professor breaks

static int students_in_office; //Total numbers of students currently in the office
static int classa_inoffice; //Total numbers of students from class A currently in the office
static int classb_inoffice; //Total numbers of students from class B in the office
static int students_since_break; //students since professor break
static int classa_students_sinceclassb; //students from class A since a student from class B
static int classb_students_sinceclassa; //students from class B since a student from class A
static int classa_students; //total students from class A
static int classb_students; //total students from class B

typedef struct 
{
  int arrival_time;  // time between the arrival of this student and the previous student
  int question_time; // time the student needs to spend with the professor
  int student_id;
  int class;
} student_info;

/* Called at beginning of simulation.  
 * 1st param: array of student structs
 * 2nd param: file passed in
 * returns: int of total students
 */
static int initialize(student_info *si, char *filename) 
{
  students_in_office = 0;
  classa_inoffice = 0;
  classb_inoffice = 0;
  students_since_break = 0;
  classa_students_sinceclassb = 0;
  classb_students_sinceclassa = 0;
  classa_students = 0;
  classb_students = 0;

  sem_init(&mutex, 0, 3); 

  /* Read in the data file and initialize the student array */
  FILE *fp;

  if((fp=fopen(filename, "r")) == NULL) 
  {
    printf("Cannot open input file %s for reading.\n", filename);
    exit(1);
  }

  int i = 0;
  while ( (fscanf(fp, "%d%d%d\n", &(si[i].class), &(si[i].arrival_time), &(si[i].question_time))!=EOF) && 
           i < MAX_STUDENTS ) 
  {
    if(si[i].class == CLASSA)
    {
      classa_students++;
    }
    else
    {
      classb_students++;
    }
    i++;
  }

 fclose(fp);
 return i;
}

/* Code executed by professor to simulate taking a break 
 * No parameters and returns void  
 */
static void take_break() 
{
  printf("The professor is taking a break now.\n");
  sleep(5);
  assert( students_in_office == 0 );
  students_since_break = 0;
}

/* Code for the professor thread.
 */
void *professorthread(void *junk) 
{
  printf("The professor arrived and is starting his office hours\n");
  sem_wait(&mutex);

  /* Loop while waiting for students to arrive. */
  while (1) 
  {
    // mutex for professor thread -- lock the mutex
    pthread_mutex_lock(&professorLock);
      if (students_since_break >= 10)
      {
        // while loop that waits while class a in office > 0 || class b in office > 0
        while (classa_inoffice > 0 || classb_inoffice > 0)
        {}
        take_break();
      }
    pthread_mutex_unlock(&professorLock);
  }
  pthread_exit(NULL);
}

/* Checks if thread meets criteria to recieve semaphore.
 * 1st param: type of class student is from
 * returns: int to show if thread meets criteria or not
 */

/* integer to take in trylock value and decide whether to 
   make threads sleep or execute perm code */
int result;

int checkperm(int type)
{
  int retval = 0;
  if(classb_students == 0)
  {
    classa_students_sinceclassb=0;
  }
  else if(classa_students==0)
  {
    classb_students_sinceclassa=0;
  }

  /* check if the mutex is not already locked and store value in result */
  result = pthread_mutex_trylock(&professorLock);
  if( result == 0) 
  {
    /* if class A thread then iterate proper variables */
    if(type==CLASSA && classb_inoffice==0
    && classa_students_sinceclassb < 5)
    {
      sem_wait(&mutex);
      students_in_office += 1;
      students_since_break += 1;
      classa_inoffice += 1;
      classa_students_sinceclassb +=1;
      classb_students_sinceclassa=0;
      classa_students--;
      retval =  1;
    }
    /* if class B thread then iterate proper variables */
    else if(type==CLASSB && classa_inoffice==0 
    && classb_students_sinceclassa < 5)
    {
      sem_wait(&mutex);
      students_in_office += 1;
      students_since_break += 1;
      classb_inoffice += 1;
      classb_students_sinceclassa += 1;
      classa_students_sinceclassb=0;
      classb_students--;
      retval =  1;
   }
   /* unlock the mutex so the next thread can enter */
    pthread_mutex_unlock(&professorLock);
  }
  /* if mutex in use then sleep before trying again -- 
     this fixed an issue with the professor taking a break at 
     inappropriate times */
  else if (result != 0)
  {
    sleep(1);
  }
  return retval;
}

/* Code executed by a class A student to enter the office.
 * Continously loops to checkperm() function
 */
void classa_enter() 
{
  while(checkperm(CLASSA)!=1)
  {}
}

/* Code executed by a class B student to enter the office.
 * Continously loops to checkperm() function
 */
void classb_enter() 
{
  while(checkperm(CLASSB)!=1)
  {}
}

/* Code executed by a student to simulate the time he spends in the office asking questions
 * 1st param: how long student asks questions
 * returns: void 
 */
static void ask_questions(int t) 
{
  sleep(t);
}

/* Code executed by a class A student when leaving the office.
 * Unlocks semaphore from thread that locks it.
 */
static void classa_leave() 
{
  students_in_office -= 1;
  classa_inoffice -= 1;
  sem_post(&mutex);
}

/* Code executed by a class A student when leaving the office.
 * Unlocks semaphore from thread that locks it.
 */
static void classb_leave() 
{
  students_in_office -= 1;
  classb_inoffice -= 1;
  sem_post(&mutex);
}

void* classa_student(void *si) 
{
  student_info *s_info = (student_info*)si;
  /* enter office */
  classa_enter();

  printf("Student %d from class A enters the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classb_inoffice == 0 );
    
  /* ask questions  --- do not make changes to the 3 lines below*/
  printf("Student %d from class A starts asking questions for %d minutes\n", s_info->student_id, s_info->question_time);
  ask_questions(s_info->question_time);
  printf("Student %d from class A finishes asking questions and prepares to leave\n", s_info->student_id);

  /* leave office */
  classa_leave();  

  printf("Student %d from class A leaves the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);

  pthread_exit(NULL);
}

void* classb_student(void *si) 
{
  student_info *s_info = (student_info*)si;

  /* enter office */
  classb_enter();

  printf("Student %d from class B enters the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);
  assert(classa_inoffice == 0 );

  printf("Student %d from class B starts asking questions for %d minutes\n", s_info->student_id, s_info->question_time);
  ask_questions(s_info->question_time);
  printf("Student %d from class B finishes asking questions and prepares to leave\n", s_info->student_id);

  /* leave office */
  classb_leave();        

  printf("Student %d from class B leaves the office\n", s_info->student_id);

  assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
  assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
  assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);

  pthread_exit(NULL);
}

/* Main function sets up simulation and prints report
 * at the end.
 * GUID: 355F4066-DA3E-4F74-9656-EF8097FBC985
 */
int main(int nargs, char **args) 
{
  int i;
  int result;
  int num_students;
  int student_countdown;
  void *status;
  pthread_t professor_tid;
  pthread_t student_tid[MAX_STUDENTS];
  student_info s_info[MAX_STUDENTS];

  if (nargs != 2) 
  {
    printf("Usage: officehour <name of inputfile>\n");
    return EINVAL;
  }

  num_students = initialize(s_info, args[1]);
  student_countdown=num_students;
  if (num_students > MAX_STUDENTS || num_students <= 0) 
  {
    printf("Error:  Bad number of student threads. "
           "Maybe there was a problem with your input file?\n");
    return 1;
  }

  printf("Starting officehour simulation with %d students ...\n",
    num_students);

  result = pthread_create(&professor_tid, NULL, professorthread, NULL);

  if (result) 
  {
    printf("officehour:  pthread_create failed for professor: %s\n", strerror(result));
    exit(1);
  }

  for (i=0; i < num_students; i++) 
  {
    s_info[i].student_id = i;
    sleep(s_info[i].arrival_time);

    if (s_info[i].class == CLASSA)
    {
      result = pthread_create(&student_tid[i], NULL, classa_student, (void *)&s_info[i]);
    }
    else // student_type == CLASSB
    {
      result = pthread_create(&student_tid[i], NULL, classb_student, (void *)&s_info[i]);
    }

    if (result) 
    {
      printf("officehour: thread_fork failed for student %d: %s\n", 
            i, strerror(result));
      exit(1);
    }
  }
  /* wait for all student threads to finish */
  for (i = 0; i < num_students; i++) 
  {
    pthread_join(student_tid[i], &status);
  }
  sem_post(&mutex);
  sem_destroy(&mutex);
  pthread_mutex_unlock(&professorLock);
  pthread_mutex_destroy(&professorLock);
  
  /* tell the professor to finish. */
  pthread_cancel(professor_tid);

  printf("Office hour simulation done.\n");

  return 0;
}
