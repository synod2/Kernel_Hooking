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
		printf("this is child3\n");
	}
};

class Child2 : public Child {
public:
	virtual void show1() {
		printf("this is child11 \n");
	}
};

int main() {
	Parent* p = new Parent;
	Child* c = new Child;
	Child* c2 = new Child;
	
	p->show1();
	p->show2();
	p->show3();

	c->show1();
	c->show3();

	c2->show1();



}