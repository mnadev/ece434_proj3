#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

pthread_mutex_t teach_mutex;
pthread_mutex_t learn_mutex;
pthread_mutex_t parent_mutex;

int expected_num_teachers;
int expected_num_students;
int expected_num_parents;
int num_teachers;
int num_students;
int num_parents;
int ratio;

void teacher_enter() {
    pthread_mutex_lock(&teach_mutex);
    num_teachers++;
    pthread_mutex_unlock(&teach_mutex);
}

int teacher_exit() {
    pthread_mutex_lock(&teach_mutex);
    int newNumTeachers = num_teachers - 1;
    if(num_students == 0) {
        num_teachers--;
        pthread_mutex_unlock(&teach_mutex);
        return 1;
    }

    if(((float)newNumTeachers/(float)num_students) > ratio) {
        num_teachers--;
        pthread_mutex_unlock(&teach_mutex);
        return 1;
    }

    pthread_mutex_unlock(&teach_mutex);
    return 0;
}


void child_enter() {
    pthread_mutex_lock(&learn_mutex);
    num_students++;
    pthread_mutex_unlock(&learn_mutex);
}

void child_exit() {
    pthread_mutex_lock(&learn_mutex);
    num_students--;
    pthread_mutex_unlock(&learn_mutex);
}

void parent_enter() {
    pthread_mutex_lock(&parent_mutex);
    num_parents++;
    pthread_mutex_unlock(&parent_mutex);
}

void parent_exit() {
    pthread_mutex_lock(&parent_mutex);
    num_parents--;
    pthread_mutex_unlock(&parent_mutex);
}

void go_home(char * person) {
    printf("%s Exiting\n", person);
    pthread_exit(0);
}

void teach() {
    printf("Teaching.\n");
}

void Teacher() {
    teacher_enter();
    // critical section
    teach();
    while ( !teacher_exit() ) {
        teach();
    }
    go_home("Teacher");
}

void learn() {
    printf("Learning.\n");
}

void Student() {
    child_enter();
    // critical section
    learn();
    child_exit();
    go_home("Student");
}

void verify_compliance() {
    if (num_teachers == 0) {
        if (num_students > 0) {
            printf("No teachers present. RIOT.\n");
        } else {
            printf("Everything up to code.\n");
        }
        return;
    }
    if( ( ((float)num_students) / ((float)num_teachers)  )  >= ratio) {
        printf("Everything up to code.\n");
    } else {
        printf("Teachers are not compliant. RIOT.\n");
    }
}

void Parent() {
    parent_enter();
    // critical section
    verify_compliance();
    parent_exit();
    go_home("Parents");
}


int main(int argc, char ** argv) {
    if (argc < 5) {
        printf("Too few arguments.\n");
        return -1;
    }

    pthread_mutex_init(&teach_mutex, NULL);
    pthread_mutex_init(&learn_mutex, NULL);
    pthread_mutex_init(&parent_mutex, NULL);

    expected_num_teachers = atoi(argv[1]);
    expected_num_students = atoi(argv[2]);
    expected_num_parents = atoi(argv[3]);

    num_teachers = 0;
    num_students = 0;
    num_parents = 0;

    ratio = atoi(argv[4]);

    pthread_t teachers[expected_num_teachers];
    pthread_t students[expected_num_students];
    pthread_t parents[expected_num_parents];

    int i = 0;
    for(i = 0; i < expected_num_teachers; i++) {
        pthread_create(&teachers[i], NULL, Teacher, NULL); 
    }

    int j = 0;
    for(j = 0; j < expected_num_students; j++) {
        pthread_create(&students[j], NULL, Student, NULL); 
    }

    int k = 0;
    for(k = 0; k < expected_num_parents; k++) {
        pthread_create(&parents[k], NULL, Parent, NULL); 
    }


    j = 0;
    for(j = 0; j < expected_num_students; j++) {
        pthread_join(students[j], NULL); 
    }

    i = 0;
    for(i = 0; i < expected_num_teachers; i++) {
        pthread_join(teachers[i], NULL); 
    }

    k = 0;
    for(k = 0; k < expected_num_parents; k++) {
        pthread_join(parents[k], NULL); 
    }

    pthread_mutex_destroy(&teach_mutex);
    pthread_mutex_destroy(&learn_mutex);
    pthread_mutex_destroy(&parent_mutex);
    return 0;
}