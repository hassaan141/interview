#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include "behaviortree_cpp_v3/action_node.h"
#include "behaviortree_cpp_v3/bt_factory.h"
using namespace std;
using namespace chrono_literals;

//For each condition node we will use a function and for each action node will use a class that inherits from syncActionNode
//For our demo, all condition will tick failure, so we go to the action node which will return true after a delay

// find ball subtree
BT::NodeStatus ballFound(){
    cout<<"Ball not found"<<endl;
    return BT::NodeStatus::FAILURE;
}

class FindBall : public BT::SyncActionNode{
    
    public:
        //Will change constructor because we are using an ouput ports in a node
        explicit FindBall(const std::string &name, const BT::NodeConfiguration &config) 
            : BT::SyncActionNode(name, config){}

        //Static method so the node knows about our ports
        static BT::PortsList providedPorts(){
            return {BT::OutputPort<vector<int>>("ball_location")};
        }

        //want to send info through our tick method, as when the port is ticked
        BT::NodeStatus tick() override{
            std::this_thread::sleep_for(3s);
            //creating a vector to send info
            vector<int> ballLocation{1, 2, 3};
            //This is how we send information using an outpput port
            BT::TreeNode::setOutput("ball_location", ballLocation);
            cout<<"Ball is found"<<endl;
            return BT::NodeStatus::SUCCESS;
        }
};

// approach ball subtree
//Need to pass in this output to use an input port
BT::NodeStatus ballApproach(BT::TreeNode &self){

    //create a message using type bt optional and using templates, make it use vector
    //After the equal sign, that line is reading info from the port
    //Used self so we can get access to the tree
    BT::Optional<vector<int>> msg = self.getInput<vector<int>>("ball_location");

    //throw error if no message exists
    if (!msg){
        throw BT::RuntimeError("missing required input[message]: ", msg.error());
    }

    //If message exists, we want to print out the value from the vector

    for(const auto position_cord : msg.value()){
        cout<<position_cord << " ";
    }

    cout<<"Ball location is far away"<<endl;
    return BT::NodeStatus::FAILURE;
}

class ApproachBall : public BT::SyncActionNode{
    
    public:
        explicit ApproachBall(const std::string &name, const BT::NodeConfiguration &config) 
            : BT::SyncActionNode(name, config){}

        //Static method so the node knows about our ports
        static BT::PortsList providedPorts(){
            return {BT::InputPort<vector<int>>("ball_location")};
        }
        
        BT::NodeStatus tick() override{

            BT::Optional<vector<int>> msg = getInput<vector<int>>("ball_location");

            if (!msg){
                throw BT::RuntimeError("missing required input[message]: ", msg.error());
            }

            //If no error, we will print elements that we recieved
            for(const auto position_cord : msg.value()){
                cout<<position_cord << " ";
            }

            std::this_thread::sleep_for(3s);
            cout<<"Ball is close"<<endl;
            return BT::NodeStatus::SUCCESS;
        }

        
};

// grasp ball subtree
BT::NodeStatus graspBall(){
    cout<<"Ball is not grasped"<<endl;
    return BT::NodeStatus::FAILURE;
}

class GraspBall : public BT::SyncActionNode{
    
    public:
        explicit GraspBall(const std::string &name) : BT::SyncActionNode(name, {}){}

        BT::NodeStatus tick() override{
            std::this_thread::sleep_for(3s);
            cout<<"Ball is grassped now"<<endl;
            return BT::NodeStatus::SUCCESS;
        }
};

// approach bin subtree
BT::NodeStatus approachBin(){
    cout<<"Bin is not close"<<endl;
    return BT::NodeStatus::FAILURE;
}

class ApproachBin : public BT::SyncActionNode{
    
    public:
        explicit ApproachBin(const std::string &name) : BT::SyncActionNode(name, {}){}

        BT::NodeStatus tick() override{
            std::this_thread::sleep_for(3s);
            cout<<"Bin is close now"<<endl;
            return BT::NodeStatus::SUCCESS;
        }
};

// place ball subtree
BT::NodeStatus dropBall(){
    cout<<"Ball is not dropped"<<endl;
    return BT::NodeStatus::FAILURE;
}

class DropBall : public BT::SyncActionNode{
    
    public:
        explicit DropBall(const std::string &name) : BT::SyncActionNode(name, {}){}

        BT::NodeStatus tick() override{
            std::this_thread::sleep_for(3s);
            cout<<"Ball is dropped now"<<endl;
            return BT::NodeStatus::SUCCESS;
        }
};

// ask for help

class AskHelp : public BT::SyncActionNode{
    
    public:
        explicit AskHelp(const std::string &name) : BT::SyncActionNode(name, {}){}

        BT::NodeStatus tick() override{

            cout<<"Asking for help. Wating for 10 seconds."<<endl;
            std::this_thread::sleep_for(10s);
            return BT::NodeStatus::SUCCESS;
        }
};


int main() {

    BT::BehaviorTreeFactory factory;

    //find ball subtree
    factory.registerSimpleCondition("BallFound", std::bind(ballFound));
    factory.registerNodeType<FindBall>("FindBall");

    //approach ball subtree
    BT::PortsList say_something_ports = {BT::InputPort<std::vector<int>>("ball_location")};
    factory.registerSimpleCondition("BallClose", ballApproach, say_something_ports);
    factory.registerNodeType<ApproachBall>("ApproachBall");

    //grasp ball subtree
    factory.registerSimpleCondition("BallGrabbed", std::bind(graspBall));
    factory.registerNodeType<GraspBall>("GrapBall");

    //approach bin subtree
    factory.registerSimpleCondition("BinClose", std::bind(approachBin));
    factory.registerNodeType<ApproachBin>("ApproachBin");

    //place ball subtree
    factory.registerSimpleCondition("BallPlaced", std::bind(dropBall));
    factory.registerNodeType<DropBall>("PlaceBall");

    //ask for help
    factory.registerNodeType<AskHelp>("AskHelp");

    //create tree
    auto tree = factory.createTreeFromFile("./../bt_tree.xml");
    
    //tick node
    tree.tickRoot();


    return 0;
}