#include <stdio.h>

class Parent {
public:
	virtual void show() {
		printf("this is parent\n");
	}
};

class Child : public Parent {
public:
	virtual void show() {
		printf("this is child\n");
	}
};

class Child2 : public Child {
public:
	virtual void show() {
		printf("this is child2 \n");
	}
};

void (Parent::*ptr)(void);

int main() {
	Parent* p = new Parent;
	Child* c = new Child;
	Child2* c2 = new Child2;
	
	p->show();
	c->show();
	c2->show();

	p = c;
	p->show();
	p = c2;
	p->show();
}