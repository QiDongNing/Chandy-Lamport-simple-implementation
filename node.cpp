#include "node.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <thread>

#include "client.h"
#include "server.h"
#include "utility.h"

using namespace std;

string Node::SerializeMessage(struct message_between_nodes m)
{
    string msg(1, m.identifier);
    msg += "|";
    msg += to_string(m.value) + "|";
    msg += to_string(m.id);

    return msg;
}

string Node::SerializeMessage(struct message_to_judge m)
{
    string msg;
    msg += to_string(m.value) + "|";
    msg += to_string(m.src_id) + "|";
    msg += to_string(m.recv_id);

    return msg;
}

struct message_between_nodes Node::DeserializeMessage(string msg)
{
    struct message_between_nodes m;
    vector<string> result = split(msg, '|');
    m.identifier = result[0].at(0);
    m.value = stoi(result[1]);
    m.id = stoi(result[2]);

    return m;
}

struct message_to_judge Node::DeserializeJudgeMessage(string msg)
{
    struct message_to_judge m;
    vector<string> result = split(msg, '|');
    m.value = stoi(result[0]);
    m.src_id = stoi(result[1]);
    m.recv_id = stoi(result[2]);

    return m;
}

Node::Node(int port, vector<string> &input)
{
    this->port = port;
    this->num_nodes = 4; //直接写死了四个
    this->balance = get_random(100, 1000);
    printf("initial balance: %d\n", this->balance);

    // vector<string> result = split(input[0], ' ');
    //  this->id = stoi(result[0]);
    //   this->snapshot_delay = stoi(result[4]);
    //   this->max_number = stoi(result[5]);

    // this->timestamps.assign(this->num_nodes, 0);
    SetNodes(input);

    string file = "config-" + to_string(this->id) + ".txt";
    {
        write_file(file, to_string(this->balance) + "\n");
    }

    // this->num_incoming_channels = 0;
    this->num_markers_received = 0;
    this->channel_states.assign(this->num_nodes, {}); // X = empty
    this->channel_is_recording.assign(this->num_nodes, false);
};

void Node::SendMessage(int conn_fd, string msg, int num_bytes)
{
    printf("send Message: %s\n", msg.c_str());
    if (send(conn_fd, msg.c_str(), num_bytes, 0) == -1)
    {
        perror("send");
    }
}

string Node::ReceiveMessage(int conn_fd, int num_bytes)
{
    char buf[1024];
    if (recv(conn_fd, buf, num_bytes, 0) == -1)
    {
        perror("server recv");
        return NULL;
    }
    return string(buf);
}

void Node::SetNodes(vector<string> &input)
{
    vector<string> result;
    for (int i = 0; i < this->num_nodes; i++)
    {
        result = split(input[i], ' ');
        // printf("Result[%d]: %s == %s\n", i, result[1].c_str(),this->hostname.c_str());

        if (stoi(result[1]) == this->port)
            this->id = i;

        this->nodes.emplace_back(stoi(result[0]), stoi(result[1])); // enplace_back相比push_back多了构造功能
    }
}

void Node::IncrementMarkersReceived(void)
{
    lock_guard<mutex> markers_received_lck(this->num_markers_received_mtx);
    this->num_markers_received++;
}

bool Node::IsFirstMarker(void)
{
    return (this->id != 0 && this->num_markers_received == 1) ? true : false;
}

void Node::StopRecordingChannel(int channel_id)
{
    unique_lock<mutex> channel_is_recording_lck(this->channel_is_recording_mtx);
    this->channel_is_recording[channel_id] = false;
}

void Node::StartRecordingChannels(int channel_id)
{
    for (int i = 0; i < this->channel_is_recording.size(); i++)
    {
        unique_lock<mutex> channel_is_recording_lck(this->channel_is_recording_mtx);
        if (i == channel_id)
            this->channel_is_recording[i] = false;
        else
            this->channel_is_recording[i] = true;
    }
}

void Node::SetNeighbours(vector<string> &input)
{
    int neighbour_id;
    pair<int, int> neighbour; //(id,port)

    // struct hostent *he;
    struct sockaddr_in client_addr;

    int n = input.size();
    // int n = 4;
    vector<string> result;

    for (int i = 0; i < n; i++)
    {
        if (i != this->id)
        {
            result = split(input[i], ' ');
            neighbour_id = stoi(result[0]);
            neighbour = this->nodes[neighbour_id];
            /*
            if ((he = gethostbyname(neighbour.first.c_str())) == NULL)
            {
                perror("gethostbyname");
                exit(1);
            }
            */
            memset(&client_addr, 0, sizeof(client_addr));
            client_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

            // memcpy(&client_addr.sin_addr, he->h_addr_list[0], he->h_length);
            client_addr.sin_port = htons(neighbour.second);
            client_addr.sin_family = AF_INET;

            int conn_fd = socket(AF_INET, SOCK_STREAM, 0);
            connect(conn_fd, (struct sockaddr *)&client_addr, sizeof(client_addr));

            this->neighbours.emplace_back(neighbour_id, conn_fd);

            printf("Client %d: connected to %d:%d\n", conn_fd, neighbour.first, neighbour.second);
        }
    }
}

void Node::AddMessageToChannelStates(int channel_id, string msg)
{
    unique_lock<mutex> channel_states_lck(this->channel_states_mtx);
    this->channel_states[channel_id].push_back(msg);
    
    //struct message_between_nodes m;
    //m = DeserializeMessage(msg);
    string file = "config-" + to_string(this->id) + ".txt";
    {
        write_file(file, msg + "\n");
    }
}

void Node::JudgerHandleServerConnection(int conn_fd)
{
    string msg;
    struct message_to_judge m;
    string file = "config-" + to_string(this->id) + ".txt";
    int temp = 0;

    while (true)
    {
        // printf("Server: total messages: %d\n", this->total_messages);

        msg = ReceiveMessage(conn_fd, 512);
        m = DeserializeJudgeMessage(msg);
        // printf("\nServer %d: received message: %s ", conn_fd, msg.c_str());
        if(m.src_id == m.recv_id )
        {
            if(temp == 0){
            write_file(file, to_string(m.src_id) + "的初值为" + to_string(m.value) + "\n");
            temp++;
            }
            else{
                write_file(file, to_string(m.src_id) + "快照时的余额为" + to_string(m.value) + "\n");
            }
        }
        else{
            write_file(file, "from"+to_string(m.src_id) + "to" + to_string(m.recv_id) +":"+to_string(m.value)+ "\n");
        }
    }
}

void Node::HandleServerConnection(int conn_fd)
{
    string msg;
    struct message_between_nodes m;

    while (true)
    {
        // printf("Server: total messages: %d\n", this->total_messages);

        msg = ReceiveMessage(conn_fd, 512);
        // printf("\nServer %d: received message: %s ", conn_fd, msg.c_str());

        m = DeserializeMessage(msg);
        printf("msg: %s\n", msg.c_str());

        // handling markers and application messages
        if (m.identifier == '1')
        {
            printf("recv marker message\n");
            IncrementMarkersReceived();
            if (IsFirstMarker())
            {
                // Set current incoming channel to NULL
                // Start recording other incoming channels
                // Send marker to outgoing neighbours
                StartRecordingChannels(m.id);
                SendMarkerMessages();
            }
            else
            {
                // output channel state and stop recording channel
                StopRecordingChannel(m.id);
            }
        }
        else
        {
            if (this->channel_is_recording[m.id] == true)
            { // in-transit
                AddMessageToChannelStates(m.id, msg);
            }

            //lock_guard<mutex> balance_lck(this->balance_mtx);
            this->balance += m.value; // add money to balance
            printf("current balance %d\n", this->balance);
            //printf("renceive money: ");
        }
    }
}

void Node::JudgeRunServer(int port)
{
    Server server(port);

    vector<thread> client_threads;
    //3ge
    // struct hostent *he;
    // string peer_name;
    while (true)
    {
        int conn_fd =
            server.Accept(server.listener_fd_, (struct sockaddr *)&server.addr_,
                          &server.addr_size_);

        client_threads.emplace_back(&Node::JudgerHandleServerConnection, this, conn_fd);
    }

    for (auto &client_thread : client_threads)
        client_thread.join();
}

void Node::RunServer(int port)
{
    Server server(port);

    vector<thread> client_threads;

    // struct hostent *he;
    // string peer_name;
    while (true)
    {
        int conn_fd =
            server.Accept(server.listener_fd_, (struct sockaddr *)&server.addr_,
                          &server.addr_size_);

        client_threads.emplace_back(&Node::HandleServerConnection, this, conn_fd);
    }

    for (auto &client_thread : client_threads)
        client_thread.join();
}

void Node::RunClient(void)
{
    int neighbour_conn;
    //neighbour_conn = neighbours[2].second;
    //ReportToJudger(neighbour_conn);
    //while (true)
    {
        neighbour_conn = neighbours[0].second;
        HandleClientConnection(neighbour_conn);

        neighbour_conn = neighbours[1].second;
        HandleClientConnection(neighbour_conn);
    }
}



void Node::HandleClientConnection(int conn_fd)
{
    // this_thread::sleep_for(chrono::milliseconds(min_send_delay));
    this_thread::sleep_for(chrono::seconds(2));
    struct message_between_nodes m;
    m.identifier = '0';
    m.id = this->id;

    //lock_guard<mutex> balance_lck(this->balance_mtx);
    m.value = (get_random(10, 80)) / 100.0 * (this->balance);
    this->balance -= m.value;

    string msg = SerializeMessage(m);

    //printf("send money: ");
    SendMessage(conn_fd, msg, 512);
}

void Node::SendMarkerMessages(void)
{
    struct message_between_nodes m;
    m.value = 0;
    m.id = this->id;
    m.identifier = '1';
    string msg;
    int conn_fd;

    string file = "config-" + to_string(this->id) + ".txt";
    {
        unique_lock<mutex> balance_lck(this->balance_mtx);
        write_file(file, to_string(this->balance) + "\n");
    }

  //  this_thread::sleep_for(chrono::seconds(10));
    for (int i = 0; i < 2; i++)
    {
        msg = SerializeMessage(m);

        conn_fd = this->neighbours[i].second;
        printf("Node %d sending Marker %s to Neighbour %d\n", this->id, msg.c_str(),
               conn_fd);
        SendMessage(conn_fd, msg, 512);
    }
}

void Node::ReportToJudger()
{   
    vector<string> result;
    int conn_fd = neighbours[2].second;
    string file = "config-" + to_string(this->id) + ".txt";
    vector<string> lines = read_file(file);
    int n = lines.size();
    struct message_to_judge m;
    // src_id和recv_id相同表示自己的状态
    for (int i = 0; i < 2; i++)
    {
        result = split(lines[i], ' ');
        m.value = stoi(result[0]);
        m.src_id = this->id;
        m.recv_id = this->id;
        string msg = SerializeMessage(m);
        SendMessage(conn_fd, msg, 512);
    }

    for (int i = 2; i < n; i++)
    {
        result = split(lines[i], '|');   
        m.value = stoi(result[1]);
        m.src_id = stoi(result[2]);
        m.recv_id = this->id;
        string msg = SerializeMessage(m);
        SendMessage(conn_fd, msg, 512);
    }
}

int main(int argc, char *argv[])
{

    if (argc != 3)
    {
        printf("Usage: ./node <id> <port>\n");
        exit(1);
    }
    printf("./node <%s> <%s>\n", argv[1], argv[2]);

    vector<string> lines = read_file("config.txt"); //配置文件 格式为id\tport\n

    // int id = stoi(argv[1]);
    int port = stoi(argv[2]);

    Node *node = new Node(port, lines);

    // node->DisplayStatus();

    if (node->id == 3)
    {
        printf("I am the judger!\n");
        thread Judger_server_thread(&Node::JudgeRunServer, node, node->port);
        Judger_server_thread.join();
        return 0;
    }

    thread server_thread(&Node::RunServer, node, node->port);

    this_thread::sleep_for(chrono::seconds(10)); //阻塞当前线程一段时间.睡眠一会等待所有节点都开启


    node->SetNeighbours(lines);

    this_thread::sleep_for(chrono::seconds(5));
        // id为0的节点作为快照的发起者,快照只做一轮
    if (node->id == 0)
    {
        this_thread::sleep_for(chrono::seconds(5));
        thread snapshot_thread(&Node::SendMarkerMessages, node);
        snapshot_thread.join();
    }

    thread client_thread(&Node::RunClient, node);

    this_thread::sleep_for(chrono::seconds(25));

    node->ReportToJudger();

    client_thread.join();
    server_thread.join();

    // node->DisplayTimestamps("Final");
    return 0;
}
