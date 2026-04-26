#include <iostream>
#include <string>
#include <vector>
using namespace std;

//class- bluprint of an object
//object- instance of a class
//instance- another name for an object
//members- variables in a class  

//abstraction: Make easy by hiding the complicated stuff
//encapsulation: Accessing private data through public methods in class only
//inheritance: Make children classes to inherit properties from parent classes
//polymorphism: Treat multiple objects as their base object type 


class Person {
    
    protected:
        string first;
        string last;

    public: 

        // Person(string first, string last){
        //     this->first = first;
        //     this->last = last;
        // }

        //New data constructor
        Person (string first, string last): first(first), last(last){}

        //New default constructor
        Person() = default;

        void printName () {
            cout<<"The full name is " << first << " " << last <<endl; 
        }

        void setFirst (string first){
            this->first = first;
        }

        void setLast (string last){
            this->last = last;
        }

        string getName(){
            return first + " " + last;
        }

        virtual void protectedLine(){
            cout<< "Using protected the name is " + first + " " + last <<endl;
        }

        //Not tied to a specific instance so thats why there is static
        static void printPeople(vector<Person*> people){
        
            for(auto person: people){
                person->protectedLine();
            }

        }



        
};

//Inheriting from the Person class, when inheriting, we need a public keyword as all other public methods from the class are private to us
class Employee : public Person{

    private:
        string job;
    
    public: 

        // Employee(string first, string last, string job) : Person(first, last){
        //     this->job = job;
        // }

        //Need a smart constructor which also inherits from parent constuctor
        Employee(string first, string last, string job): Person(first, last), job(job){};
        
        //Smart default constructor
        Employee() = default;

        //OG default constructor
        // Employee() : Person("", ""){
        //     this->job= "Unemployed";
        // }

        //Without using protected using a getter for our parent class
        string printInfo(){
            string info = "The full name is " + getName() + " and the job is " + job; 
            return info;
        }
        
        //We can also explicitly override using override and virtual
        void protectedLine() override{
            cout<< "Using protected the name is " + first + " " + last + " and mashallah he is an " + job <<endl;
        }

        //If we want child class to access parent class members, use protected

};  

int main () {

    Person a("Muhammad", "Farooqi");
    a.printName();

    Person b;
    b.printName();
    
    //Inheritance From Parent Class

    Employee e;
    e.setFirst("Mohammad");
    e.setLast("Nassar");
    e.printName();

    //Inheritance default and data constructor

    Employee d("Someguy", "Otherguy", "Imam");
    d.printName();
    cout<< d.printInfo() <<endl;

    //Overriding and Protected
    
    a.protectedLine();
    d.protectedLine();

    //Polymorphism
    cout<<"\n"<<endl;
    cout<<"Now we are going to do polymorphism\n" <<endl;

    //Even though e and d are Employee objects, they can be stored in a Person Vector becuase Employee is a child class of Person. This is bc of polymorphism
    vector<Person*> people;

    //Storing Person pointers instead of objects
    people.push_back(&a);
    people.push_back(&e);
    people.push_back(&d);

    //Interating a person to every element in people
    for(auto person: people){
        person->protectedLine();
    }

    //Pointers and Polymorphism
    Person* ab = new Person("Abbas", "Khan");
    cout<<ab->getName()<<endl;
    ab->setFirst("Omar");
    cout<<ab->getName()<<endl;
    
    //Static used when iterating over a non instance, so for example, all the instances of type Person
    cout<<"\nPrinting over a static type"<<endl;
    Person::printPeople(people);

    return 0;

}


//Learn pointers on objects 