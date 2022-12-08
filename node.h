#ifndef NODE_H_
#define NODE_H_

#include <string>
#include <utility>
#include <vector>
#include <mutex>
#include <tuple>

struct message_between_nodes //节点之间传输信息用的结构体
{
    char identifier; // C-L算法标识符"M"，取值只有"0"和"1",标0为正常信息，标1为标识符信息，此时value和id作废
    int value;         //传输金额
    int id;            //发送者的id
};

struct message_to_judge //发送给判决机的信息使用的结构体
{
    int value;   //金额
    int src_id;  //发送者的id
    int recv_id; //接受者的id，该数值为负数（我们规定为-1）时，该消息的内容为发送者开始做快照时本地保存的金额值，在初始化阶段也用此方式发送
};

class Node
{
public:
    Node(int port, std::vector<std::string> &input);

    void SetNodes(std::vector<std::string> &input);
    void SetNeighbours(std::vector<std::string> &input);

    void DisplayStatus(void);

    //int NumberOfMessagesToSend(void);

    void SendMessage(int conn_fd, int num);
    void SendMessage(int conn_fd, std::string msg, int num_bytes);
    int ReceiveMessage(int conn_fd);
    std::string ReceiveMessage(int conn_fd, int num_bytes);

    void HandleServerConnection(int conn_fd);
    void HandleClientConnection(int conn_fd);

    void RunServer(int port);
    void JudgeRunServer(int port);
    void RunClient(void);

    //void ChandyLamport(void);
    void SendMarkerMessages(void);
    void ReceiveMarkerMessages(bool &is_first_marker, struct Message m, int &num_markers_received, std::string msg);
    //void IncrementIncomingChannels(void);
    void IncrementMarkersReceived(void);
    bool IsFirstMarker(void);
    void AddMessageToChannelStates(int channel_id, std::string msg);
    void StartRecordingChannels(int channel_id);
    void StopRecordingChannel(int channel_id);
    void JudgerHandleServerConnection(int conn_fd);
    void ReportToJudger();

    //struct Message ConstructMessage(std::string identifier, const std::vector<int> &timestamp, int value);

    std::string SerializeMessage(struct message_between_nodes m);
    std::string SerializeMessage(struct message_to_judge m);

    struct message_between_nodes DeserializeMessage(std::string msg);
    struct message_to_judge DeserializeJudgeMessage(std::string msg);

    void DisplayChannelStates(void);

    int id;
    std::string hostname;
    int port;
    bool is_active;


private:
    int balance;
    std::mutex balance_mtx;

    int total_messages;
    std::mutex total_messages_mtx;

    int num_nodes;
    std::vector<std::pair<int, int>> nodes;//(id,port)
    std::vector<std::pair<int, int>> neighbours; // holds neighbour_id and neighbour_conn_fd


    std::vector<std::vector<std::string>> channel_states;
    std::mutex channel_states_mtx;

    std::vector<bool> channel_is_recording;
    std::mutex channel_is_recording_mtx;

    int num_incoming_channels;
    std::mutex num_incoming_channels_mtx;

    int num_markers_received;
    std::mutex num_markers_received_mtx;

    bool is_first_marker = false;
    std::mutex is_first_marker_mtx;
};

#endif // NODE_H_