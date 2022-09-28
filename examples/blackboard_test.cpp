#include "crossdoor_nodes.h"

#include "behaviortree_cpp_v3/loggers/bt_cout_logger.h"
#include "behaviortree_cpp_v3/loggers/bt_minitrace_logger.h"
#include "behaviortree_cpp_v3/loggers/bt_file_logger.h"
#include "behaviortree_cpp_v3/bt_factory.h"

#ifdef ZMQ_FOUND
#include "behaviortree_cpp_v3/loggers/bt_zmq_publisher.h"
#endif

/** This is a more complex example that uses Fallback,
 * Decorators and Subtrees
 *
 * For the sake of simplicity, we aren't focusing on ports remapping to the time being.
 *
 * Furthermore, we introduce Loggers, which are a mechanism to
 * trace the state transitions in the tree for debugging purposes.
 */


// clang-format off

static const char* xml_text = R"(
<root main_tree_to_execute = "MainTree">
	<!--------------------------------------->
    <BehaviorTree ID="DoorClosed">
        <Sequence name="door_closed_sequence">
            <Inverter>
                <Condition ID="IsDoorOpen"/>
            </Inverter>
            <RetryUntilSuccessful num_attempts="4">
                <OpenDoor/>
            </RetryUntilSuccessful>
            <PassThroughDoor/>
        </Sequence>
    </BehaviorTree>
    <!--------------------------------------->
    <BehaviorTree ID="MainTree">
        <Sequence>
            <SetBlackboard output_key="battery_status" value="1"/>
            <SetBlackboard output_key="check_valve" value="34"/>
            <SetBlackboard output_key="thermal_check" value="There"/>
            <SetBlackboard output_key="lock" value="true"/>
            <SetBlackboard output_key="safe" value="4.56"/>
            <SetBlackboard output_key="flag" value="45"/>
            <Fallback name="root_Fallback">
                <Sequence name="door_open_sequence">
                    <IsDoorOpen/>
                    <PassThroughDoor/>
                </Sequence>
                <SubTree ID="DoorClosed"/>
                <PassThroughWindow/>
            </Fallback>
            <CloseDoor/>
        </Sequence>
    </BehaviorTree>
    <!---------------------------------------> 
</root>
 )";

// clang-format on

using namespace BT;

int main(int argc, char** argv)
{
    BT::BehaviorTreeFactory factory;

    // Register our custom nodes into the factory.
    // Any default nodes provided by the BT library (such as Fallback) are registered by
    // default in the BehaviorTreeFactory.
    CrossDoor::RegisterNodes(factory);

    // Important: when the object tree goes out of scope, all the TreeNodes are destroyed
    auto tree = factory.createTreeFromText(xml_text);

    // This logger prints state changes on console
    StdCoutLogger logger_cout(tree);

    // This logger saves state changes on file
    FileLogger logger_file(tree, "bt_trace.fbl");

    // This logger stores the execution time of each node
    MinitraceLogger logger_minitrace(tree, "bt_trace.json");

#ifdef ZMQ_FOUND
    // This logger publish status changes using ZeroMQ. Used by Groot
    PublisherZMQ publisher_zmq(tree);
#endif

    printTreeRecursively(tree.rootNode());

    const bool LOOP = ( argc == 2 && strcmp( argv[1], "loop") == 0);

    using std::chrono::milliseconds;
    do
    {
        NodeStatus status = NodeStatus::RUNNING;
        // Keep on ticking until you get either a SUCCESS or FAILURE state
        while( status == NodeStatus::RUNNING)
        {   
            status = tree.tickRoot();

            // std::string key = "i";
            // Any* i = tree.blackboard_stack[0]->getAny(key);
            
            // std::cout<<"BlackBoard\n";
            // auto keys = tree.blackboard_stack[0]->getKeys();
            // std::cout<<"Does it even print this"<<"\n";
            // for(int i =0; i < keys.size(); i++)
            // {   StringView key = keys[i];
            //     std::string value = tree.blackboard_stack[0]->getAny(std::string{key})->cast<std::string>();
            //     std::cout<<key<<":"<<value<<"\n";
            // }

            // std::string output = i->cast<std::string>();
            // std::cout<<output;
            
            // IMPORTANT: you must always add some sleep if you call tickRoot()
            // in a loop, to avoid using 100% of your CPU (busy loop).
            // The method Tree::sleep() is recommended, because it can be
            // interrupted by an internal event inside the tree.
            tree.sleep( milliseconds(10) );
        }
        if( LOOP )
        {
            std::this_thread::sleep_for( milliseconds(1000) );
        }
    }
    while(LOOP);

    return 0;
}
