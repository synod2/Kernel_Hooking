#include <stdio.h>

class Parent {
public:
	virtual void show1() {
		printf("this is parent1\n");
	}
	virtual void show2() {
		printf("this is parent2\n");
	}
	virtual void show3() {
		printf("this is parent3\n");
	}
};

class Child : public Parent {
public:
	virtual void show1() {
		printf("this is child1\n");
	}
	
	virtual void show3() {
		printf("this is child1\n");
	}
};

class Child2 : public Child {
public:
	virtual void show1() {
		printf("this is child2 \n");
	}
};

int main() {
	Parent* p = new Parent;
	Child* c = new Child;
	Child2* c2 = new Child2;

	void (Parent:: * fptr1) (void) = &Parent::show1;
	void (Parent:: * fptr2) (void) = &Parent::show2;
	void (Parent:: * fptr3) (void) = &Parent::show3;

	void (Child:: * fptrc1) (void) = &Child::show1;
	void (Child:: * fptrc2) (void) = &Child::show2;
	void (Child:: * fptrc3) (void) = &Child::show3;

	void (Child2:: * fptrcc1) (void) = &Child2::show1;
	void (Child2:: * fptrcc2) (void) = &Child2::show2;
	void (Child2:: * fptrcc3) (void) = &Child2::show3;

	printf("Parent::show1 : 0x%08lx \n", fptr1);
	printf("Parent::show2 : 0x%08lx \n", fptr2);
	printf("Parent::show3 : 0x%08lx \n", fptr3);
	printf("----------------------------------\n");
	printf("Child::show1 : 0x%08lx \n", fptrc1);
	printf("Child::show2 : 0x%08lx \n", fptrc2);
	printf("Child::show3 : 0x%08lx \n", fptrc3);
	printf("----------------------------------\n");
	printf("Child2::show1 : 0x%08lx \n", fptrcc1);
	printf("Child2::show2 : 0x%08lx \n", fptrcc2);
	printf("Child2::show3 : 0x%08lx \n", fptrcc3);
}