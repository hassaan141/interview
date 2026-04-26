#include<iostream>

//creating the foundation for the linked list using the Node class
struct Node{
  int data;
  Node *next;
};

typedef Node* nodePtr;

//creating the linked list class
class LinkedList{
  public:
    LinkedList(){
      head = nullptr;
    }

    void addFront(int value){
      nodePtr traverse = new Node;
      traverse->data = value;
      traverse->next = head;
      head = traverse;
    }
    
    void addBack(int value){
    	nodePtr box = new Node();
    	nodePtr traverse = head;
    	box->data = value;
    	
    	if (head == nullptr){
    		head = box;
		} 
		else{
			
			while(traverse->next != nullptr){
    		traverse = traverse->next;
    		
			}
			traverse->next = box;
		}

	}
	
	void addToIndex(int value, int index){
		nodePtr dummy = new Node();
		dummy->data = value;
		nodePtr traverse = head;
		for (int i =0; i<index-1; i++){
			traverse = traverse->next;
		}
		dummy->next = traverse->next;
		traverse->next = dummy;
	}
	
	void removeValue(int number){
		nodePtr traverse = head;
		
		while(traverse !=nullptr && traverse->next != nullptr){
			
			if(traverse->next->data == number){
//				nodePtr temp = traverse->
				traverse->next = traverse->next->next;
			}else{
				traverse = traverse->next;
			}
			
		}
	}

  void printList(){
    nodePtr traverse = head;
    while(traverse != nullptr){
      std::cout<<traverse->data<<" ";
      traverse = traverse->next;
    }
      
    std::cout<<std::endl;
  }

  private:
    Node *head = nullptr;
};

int main (){
  LinkedList list;
  list.addBack(2);
  list.addBack(3);
  list.addBack(4);
  list.addBack(5);
  list.addBack(6);
  list.printList();
  list.addFront(1);
  list.printList();
  list.addToIndex(69, 3);
  list.printList();
  list.removeValue(4);
  list.printList();

  return 0;

}
