#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t lockTeacher; // mutex for teacher count
pthread_mutex_t lockChildren; // mutex for children count
pthread_mutex_t condChildrenMutex = PTHREAD_MUTEX_INITIALIZER; // for condition variable condChildren
pthread_cond_t condChildren = PTHREAD_COND_INITIALIZER; //to signal to teacher that a child has left.
int teacherPresent = 0;
int childrenPresent = 0;
int ratio = 0;

void goHome() {
	pthread_exit(0);
}

int teacher_enter() {
	pthread_mutex_lock(&lockTeacher);
	teacherPresent++;
	pthread_mutex_unlock(&lockTeacher);
	return 1;
}

void teacher_exit() {
	teacherPresent--;
}

int verify_compliance(int teacherPresent, int childrenPresent) {
	int valid = 1;
	//printf("Teachers present: %d\n", teacherPresent);
	//printf("Children present: %d\n", teacherPresent);
	if(!teacherPresent && childrenPresent){
		valid = 0;
	}else if(!childrenPresent) {
		valid = 1;
	}else if( ( ((float)childrenPresent) / ((float)teacherPresent)  )  <= ratio) {
        //printf("%d %d %d \n",childrenPresent,teacherPresent, ratio);
        valid = 1;
    } else {
        valid = 0;
    }
	return valid;
}

void teach() {
	while(1) {
		int breakOut = 0;
		pthread_mutex_lock(&lockTeacher);
		pthread_mutex_lock(&lockChildren);
		if(teacherPresent == 1 && childrenPresent == 0) {
			teacher_exit();
			breakOut = 1;
		}
		else if(verify_compliance(teacherPresent-1, childrenPresent)) {
			teacher_exit();
			breakOut = 1;
		}
		pthread_mutex_unlock(&lockTeacher);
		pthread_mutex_unlock(&lockChildren);
		if(breakOut) {
			break;
		}
		pthread_cond_wait(&condChildren,&condChildrenMutex); //Wait for children to leave
	}
}

void* Teacher(void * in){
	teacher_enter();
	teach();
	printf("Teacher going home!\n");
	goHome();
}

void child_enter() {
	pthread_mutex_lock(&lockChildren);
	childrenPresent++;
	pthread_mutex_unlock(&lockChildren);
}

void learn() {
	sleep((rand() % (10-5+1))+5); // Sleeps between 5 to 10 secs
	printf("I'm done learning...\n");
}

void child_exit() {
	int tempCP;
	pthread_mutex_lock(&lockChildren);
	childrenPresent--;
	pthread_mutex_unlock(&lockChildren);
	pthread_cond_signal(&condChildren);
}

void* Child(void * in) {
	child_enter();
	learn();
	child_exit();
	printf("Child going home!\n");
	goHome();
}

void parent_enter() {
	//Does nothing
	sleep((rand() % (10-1+1))+1); // Sleeps between 1 to 10 secs
}

void* Parent(void * in) {
	parent_enter();
	pthread_mutex_lock(&lockTeacher);
	pthread_mutex_lock(&lockChildren);
	if(verify_compliance(teacherPresent, childrenPresent)) {
		printf("Everything seems good!\n");
	}else{
		printf("Who's watching my kid!\n");
	}
	pthread_mutex_unlock(&lockTeacher);
	pthread_mutex_unlock(&lockChildren);
	printf("Parent going home!\n");
	goHome();
}

int main(int argc, char**argv) {
	if(argc == 5) {
		int numberOfTeachers = atoi(argv[1]);
		int numberOfChildren = atoi(argv[2]);
		int numberOfParents = atoi(argv[3]);
		ratio = atoi(argv[4]);
		int i;
		int j = 0;
		int N = numberOfTeachers + numberOfChildren + numberOfParents;
		pthread_t pthreds [N];
		for(i = 0; i < numberOfChildren; i++) {
			pthread_create(&pthreds[j],NULL,Child,NULL);
			j++;
		}
		for(i = 0; i < numberOfTeachers; i++) {
			pthread_create(&pthreds[j],NULL,Teacher,NULL);
			j++;
		}
		for(i = 0; i < numberOfParents; i++) {
			pthread_create(&pthreds[j],NULL,Parent,NULL);
			j++;
		}
		for(i = 0; i < N; i++) {
			pthread_join(pthreds[i],NULL);
		}
	}else{
		printf("Required Arguments: <NUMBER OF TEACHERS> <NUMBER OF CHILDREN> <NUMBER OF PARENTS> <RATIO>\n");
	}
	exit(0);
}