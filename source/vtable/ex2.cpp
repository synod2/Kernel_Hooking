#include <stdio.h>

class Parent {
	public : 
		void show() {
			printf("this is parent \n");
		}

		void show(int a) {
			printf("this is parent %d \n",a);
		}
};

class Child : public Parent {
	public :
		void show() {
			printf("this is Child \n");
		}

};

int main() {
	Child* c = new Child;
	Parent * p = new Parent;
	
	c->show();
	//c->show(10);
	p->show();
	p->show(10);
}