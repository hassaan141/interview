#include<iostream>

//Really good for depth first search

//stack size
const int SIZE = 50;

class Stack{

public: 
	Stack(){
		m_top = 0;
	}
	
	void push(int value){
		m_stack[m_top] = value;
		m_top++;
	}
	
    void pop() {
        if (m_top == 0) {
            std::cout << "Stack Underflow: Cannot pop from an empty stack." << std::endl;
            return;
        }
        m_top--; 
    }
	
	void print(){
		if (m_top == 0) {
			std::cout<<"Empty Stack";
			return;
		}
		
		for(int i = m_top -1; i>=0; i++){
			std::cout<<m_stack[i]<<std::endl;
		}

		
		
	}

private:
	int m_top;
	int m_stack[SIZE];
};

int main(){
	
}