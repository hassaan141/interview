// https://www.behaviortree.dev/docs/nodes-library/DecoratorNode

#include <iostream>
#include <chrono>
#include <string>
#include "behaviortree_cpp_v3/action_node.h"
#include "behaviortree_cpp_v3/bt_factory.h"


using namespace std;
using namespace chrono_literals;


// Node class inheriting from SyncActionNode
class ApproachObject : public BT::SyncActionNode {
public:
    // Explicit constructor using the name of the node
    explicit ApproachObject(const string &name) : BT::SyncActionNode(name, {}) {}

    // Override tick method
    BT::NodeStatus tick() override {
        //Adding the name identifier from the xml file
        cout << "Approach Object: " << this->name() << endl;
        this_thread::sleep_for(5s);
        return BT::NodeStatus::SUCCESS;
    }
};

// Function
BT::NodeStatus CheckBattery() {
    std::cout << "Battery OK" << std::endl;
    return BT::NodeStatus::SUCCESS;
};

// Custom class for each leafnode executions: Gripper Interface
class GripperInterface {
public:
    GripperInterface() : _open(true) {}

    BT::NodeStatus open() {

        _open = true;
        cout << "Gripper Opened" << endl;
        return BT::NodeStatus::SUCCESS;
    }

    BT::NodeStatus close() {

        _open = false;
        cout << "Gripper Closed" << endl;
        return BT::NodeStatus::SUCCESS;
    }

private:
    bool _open;
};

int main() {

    //Registor and store the entire behaviour tree
    BT::BehaviorTreeFactory factory;

    //When creating a class which inherits from node
    factory.registerNodeType<ApproachObject>("ApproachObject");

    //Registor simple condition node for function
    //Use std bind because
    //The string is the name of the node
    factory.registerSimpleCondition("CheckBattery", std::bind(CheckBattery));

    //For the custom class, create an object of that class and use it
    GripperInterface gripper;

    factory.registerSimpleAction(
        "OpenGripper",
        //Used std bind to get a pointer of the method for instance gripper
        std::bind(&GripperInterface::open, &gripper)
    );

    factory.registerSimpleAction(
        "CloseGripper",
        std::bind(&GripperInterface::close, &gripper)
    );

    //Creating our tree
    auto tree = factory.createTreeFromFile("./../bt_tree.xml");

    tree.tickRoot();

    return 0;
}
