#ifndef HCQ_H
#define HCQ_H

/* Students are kept in order by time with newest 
   students at the end of the lists. */
struct student{
    char *name;
    struct course *course;
    struct student *next_overall;
};

/* Tas are kept in reverse order of their time of addition. Newest
   Tas are kept at the head of the list. */ 
struct ta{
    char *name;
    struct student *current_student;
    struct ta *next;
};

struct course{
    char code[7];
};


typedef struct student Student;
typedef struct course Course;
typedef struct ta Ta;

// find the struct student with name stu_name
Student *find_student(Student *stu_list, char *student_name);

// find the ta with name ta_name
Ta *find_ta(Ta *ta_list, char *ta_name);

// find the course with this code in the course list
Course *find_course(Course *courses, int num_courses, char *course_code);

// route around the removing student from the overall student queue
void route_around_overall(Student **stu_list_ptr, Student *thisstudent);

// add a student to the queue with student_name and a question about course_code
int add_student(Student **stu_list_ptr, char *student_name, char *course_num,
    Course *courses, int num_courses);

// remove the student from the queues and record statistics
int give_up_waiting(Student **stu_list_ptr, char *student_name);

// create and prepend TA with ta_name to the head of ta_list
void add_ta(Ta **ta_list_ptr, char *ta_name);

// remove this Ta from the ta_list
int remove_ta(Ta **ta_list_ptr, char *ta_name);

// remove the current student that the Ta ta_tofree is serving
void release_current_student(Ta *ta_tofree);

// take student to_serve out of the stu_list
int take_student(Ta *ta, Student **stu_list_ptr, Student *to_serve);

//  if student is currently being served then this finishes this student
//    if there is no-one else waiting then the currently being served gets
//    set to null 
int next_overall(char *ta_name, Ta **ta_list_ptr, Student **stu_list_ptr);

// list currently being served by current TAs
char *print_currently_serving(Ta *ta_list);

// list all students in queue 
char *print_full_queue(Student *stu_list);

// dynamically allocate space for the array course list and populate it
int config_course_list(Course **courselist_ptr, char *config_filename);

#endif
