// 访问一个文件有如下流程，首先通过根目录文件查找文件名，确定是哪一个条目，接着在条目中访问DIR_FstClus，
// 这个里面有对应的开始簇号，若是文件大于512字节，需要分好几个扇区来存储数据，当第一个扇区号访问完后，
// 通过FAT可以访问下一个簇号，对应的则是下一个扇区。
#include<bits/stdc++.h>

using namespace std;

int BytsPerSec;				//每扇区字节数
int SecPerClus;				//每簇扇区数
int RsvdSecCnt;				//Boot记录占用的扇区数
int NumFATs;				//FAT表个数
int RootEntCnt;				//根目录最大文件数
int FATSz;					//FAT扇区数

typedef unsigned char u8;	//1字节
typedef unsigned short u16;	//2字节
typedef unsigned int u32;	//4字节

#pragma pack(1) //以1字节对齐

typedef struct BPB {
    u16  BPB_BytsPerSec;	//每扇区字节数
    u8   BPB_SecPerClus;	//每簇扇区数
    u16  BPB_RsvdSecCnt;	//Boot记录占用的扇区数
    u8   BPB_NumFATs;	    //FAT表个数
    u16  BPB_RootEntCnt;	//根目录最大文件数
    u16  BPB_TotSec16;		//扇区总数
    u8   BPB_Media;		    //介质描述符
    u16  BPB_FATSz16;	    //每个FAT表所占扇区数
    u16  BPB_SecPerTrk;     //每磁道扇区数（Sector/track）
    u16  BPB_NumHeads;	    //磁头数（面数）
    u32  BPB_HiddSec;	    //隐藏扇区数
    u32  BPB_TotSec32;	    //如果BPB_ToSec16为0，该值为扇区数
} BPB;                      //25字节

typedef struct RootEntry {
    char DIR_Name[11];      //长度名+扩展名
    u8   DIR_Attr;		    //文件属性
    char reserved[10];      //保留位
    u16  DIR_WrtTime;
    u16  DIR_WrtDate;
    u16  DIR_FstClus;	    //开始簇号
    u32  DIR_FileSize;
} RootEntry;                //32字节

#pragma pack()

class Node {
public:
    string name;		    //名字
    vector<Node *> next;	//下一级目录的Node数组
    Node *front;            //记录父目录，用于处理..
    string path;			//记录path，便于打印操作
    u32 FileSize;			//文件大小
    bool isfile = false;	//是文件还是目录
    bool isval = true;		//用于标记是否是.和..
    int dir_count = 0;		//记录下一级有多少目录
    int file_count = 0;		//记录下一级有多少文件
    char *content = new char[10000];		//存放文件内容
};


void myPrint(const char *s);
void ReadDir(FILE *fat12, Node *root);
void getContent(FILE *fat12, Node *child, int startClus);
int getFATValue(FILE *fat12, int currentClus);
void HandleCat(vector<string> commands, Node *root);
void outputCat(Node *root, string path, int &judge);
void getChild(FILE *fat12, Node *child, int startClus);
void HandleLs(vector<string> commands, Node *root);
void outputLS(Node *root);
bool isL(string &command);
bool isDir(string &path);
void outputLS(Node *root, string path, int &judge);
void outputLSL(Node *root, string path, int &judge);
void outputLSL(Node *root);

// ---------------------------------------------
// functions

extern "C" {
    void my_print(const char *, int);
}

void myPrint(const char *s) {
    my_print(s, strlen(s));
}

// splite the input str
vector<string> split(const string& str, const char delimiter){
    vector<string> result;
    string token;
    for(char s : str){
        if(s == delimiter){
            if(!token.empty()){
                result.push_back(token);//push the front token
                token.clear();
            }
        }else{
            token += s;
        }
    }
    if(!token.empty()){
        result.push_back(token);
    }
    return result;
}

// 读取根目录条目并加以存储
void ReadDir(FILE *fat12, Node *root){
    int base = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec;
    RootEntry rootEntry;//用来读取32字节目录条目
    for(int i = 0; i < RootEntCnt; i++){
        char storeName[12];//用来临时存储文件名及其拓展
        fseek(fat12, base, SEEK_SET);
        fread(&rootEntry, 1, 32, fat12);
        base += 32;//读完一条偏移32字节继续读取
        int judge = 1;
        if(rootEntry.DIR_Name[0] == '\0'){
            continue;
        }
        // printf("%s %d\n", rootEntry.DIR_Name, rootEntry.DIR_Attr);
        for(int j = 0; j < 11; j++){
            if((rootEntry.DIR_Name[j] >= 'A' && rootEntry.DIR_Name[j] <= 'Z')
            || (rootEntry.DIR_Name[j] >= '0' && rootEntry.DIR_Name[j] <= '1')
            || (rootEntry.DIR_Name[j] == ' ')){
                // do nothing
            }else{
                judge = 0;
                break;
            }
        }
        if(judge == 0){
            continue;
        }
        // FAT12中名称均为大写，只要求输入为英文大写/数字的指定路径/文件名
        // （即小写的文件/目录名为不存在），不考虑中文字符，不需要支持长文件名
        // 看这要求输入都是合法的？不需要检验？
        // 判断是文件还是目录，FAT12下文件属性常见如下:
        // 0x01：只读文件。
        // 0x02：隐藏文件。
        // 0x04：系统文件。
        // 0x08：卷标。
        // 0x10：子目录。
        // 0x20：存档文件
        if((rootEntry.DIR_Attr & 0x10) == 0){ //是文件
            int index = 0;
            for(int j = 0; j < 8; j++){
                if(rootEntry.DIR_Name[j] != ' '){
                    storeName[index] = rootEntry.DIR_Name[j];
                    index++;
                }
            }
            storeName[index] = '.';
            index++;
            for(int j = 8; j < 11; j++){
                if(rootEntry.DIR_Name[j] != ' '){
                    storeName[index] = rootEntry.DIR_Name[j];
                    index++;
                }
            }
            storeName[index] = '\0';
            Node *child = new Node();
            child->name = storeName;
            child->path = root->path + storeName + "/";
            child->isfile = true;
            child->FileSize = rootEntry.DIR_FileSize;
            root->next.push_back(child);
            root->file_count++;
            getContent(fat12, child, rootEntry.DIR_FstClus);
        }else{ //是目录
            int index = 0;
            for(int j = 0; j < 11; j++){
                if(rootEntry.DIR_Name[j] != ' '){
                    storeName[index] = rootEntry.DIR_Name[j];
                    index++;
                }else{
                    break;
                }
            }
            storeName[index] = '\0';
            Node *child = new Node();
            child->name = storeName;
            child->path = root->path + storeName + "/";
            child->isfile = false;
            root->dir_count++;
            root->next.push_back(child);
            child->front = root;
            //为该目录添加.和..
            Node *special_node = new Node();
            special_node->name = ".";
            special_node->isval = false;
            special_node->path = child->path;
            child->next.push_back(special_node);
            Node *special_node1 = new Node();
            special_node1->name = "..";
            special_node1->isval = false;
            special_node1->path = root->path;
            child->next.push_back(special_node1);
            getChild(fat12, child, rootEntry.DIR_FstClus);
        }
    }
}

//将文件内容读取到Node中
void getContent(FILE *fat12, Node *child, int startClus){
    int dataBase = BytsPerSec * (RsvdSecCnt + FATSz * NumFATs + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);
    // 加上BPB_BytsPerSec-1保证此公式在根目录区无法填满整个扇区时仍然能够成立，由此得到数据区开始
    int currentClus = startClus;
    int value = 0;
    char *content = child->content;
    if(startClus == 0){
        return;
    }
    while(value < 0xFF8){
        value = getFATValue(fat12, currentClus);
        if(value == 0xFF7){
            myPrint("Wrong!\n");
            break;
        }
        int size = SecPerClus * BytsPerSec;//每簇多少字节大小数据
        char *str = new char[size];
        char *ptr = str;
        int startByte = dataBase + (currentClus - 2) * size;
        fseek(fat12, startByte, SEEK_SET);
        fread(ptr, 1, size, fat12);
        for (int i = 0; i < size; ++i) {
            *content = ptr[i];
            content++;
        }
        delete str;
        currentClus = value;
    }
}

//获取下一个簇号
int getFATValue(FILE *fat12, int currentClus){
    int FATBase = RsvdSecCnt * BytsPerSec;
    //只查看FAT1
    int pos = FATBase + currentClus * 3 / 2;
    int type = currentClus % 2;
    u16 bytes;
    u16* bytes_ptr = &bytes;
    fseek(fat12, pos, SEEK_SET);
    fread(bytes_ptr, 1, 2, fat12);
    if(type == 0){
        bytes = bytes << 4;
    }
    return bytes >> 4;
}

//进行递归，解析目录结构
void getChild(FILE *fat12, Node *root, int startClus){
    int dataBase = BytsPerSec * (RsvdSecCnt + FATSz * NumFATs + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);
    int currentClus = startClus;
    int value = 0;
    while(value < 0xFF8){
        value = getFATValue(fat12, currentClus);
        if(value == 0xFF7){
            myPrint("Wrong!\n");
            break;
        }
        int size = SecPerClus * BytsPerSec;
        int startByte = dataBase + (currentClus - 2) * size;
        int cnt = 0;
        while(cnt < size){
            char storeName[12];
            RootEntry rootEntry;
            fseek(fat12, startByte + cnt, SEEK_SET);
            fread(&rootEntry, 1, 32, fat12);
            cnt += 32;
            int judge = 1;
            // printf("%s %d\n", rootEntry.DIR_Name, rootEntry.DIR_Attr);
            if(rootEntry.DIR_Name[0] == '\0'){
                continue;
            }
            for(int j = 0; j < 11; j++){
                if((rootEntry.DIR_Name[j] >= 'A' && rootEntry.DIR_Name[j] <= 'Z')
                || (rootEntry.DIR_Name[j] >= '0' && rootEntry.DIR_Name[j] <= '9')
                || (rootEntry.DIR_Name[j] == ' ')){
                    // do nothing
                }else{
                    judge = 0;
                    break;
                }
            }
            // printf("%s %d\n", rootEntry.DIR_Name, rootEntry.DIR_Attr);
            if(judge == 0){
                continue;
            }
            // printf("%s %d\n", rootEntry.DIR_Name, rootEntry.DIR_Attr);
            if((rootEntry.DIR_Attr & 0x10) == 0){
                int index = 0;
                for(int j = 0; j < 8; j++){
                    if(rootEntry.DIR_Name[j] != ' '){
                        storeName[index] = rootEntry.DIR_Name[j];
                        index++;
                    }
                }
                storeName[index] = '.';
                index++;
                for(int j = 8; j < 11; j++){
                    if(rootEntry.DIR_Name[j] != ' '){
                        storeName[index] = rootEntry.DIR_Name[j];
                        index++;
                    }
                }
                storeName[index] = '\0';
                Node *child = new Node();
                child->name = storeName;
                child->path = root->path + storeName + "/";
                child->isfile = true;
                child->FileSize = rootEntry.DIR_FileSize;
                root->next.push_back(child);
                root->file_count++;
                getContent(fat12, child, rootEntry.DIR_FstClus);
            }
            else{
                int index = 0;
                for(int j = 0; j < 11; j++){
                    if(rootEntry.DIR_Name[j] != ' '){
                        storeName[index] = rootEntry.DIR_Name[j];
                        index++;
                    }else{
                        break;
                    }
                }
                storeName[index] = '\0';
                Node *child = new Node();
                child->name = storeName;
                child->path = root->path + storeName + "/";
                child->isfile = false;
                root->dir_count++;
                root->next.push_back(child);
                child->front = root;
                //为该目录添加.和..
                Node *special_node = new Node();
                special_node->name = ".";
                special_node->isval = false;
                special_node->path = child->path;
                child->next.push_back(special_node);
                Node *special_node1 = new Node();
                special_node1->name = "..";
                special_node1->isval = false;
                special_node1->path = root->path;
                child->next.push_back(special_node1);
                getChild(fat12, child, rootEntry.DIR_FstClus);
            }
        }
        currentClus = value;
    }
}

//处理cat命令
void HandleCat(vector<string> commands, Node *root){
    if(commands.size() == 2){
        int judge = 0;
        outputCat(root, commands[1], judge);
        if(judge == 0){
            myPrint("No such file!\n");
        }
    }else{
        myPrint("Bad Command\n");
    }
}

//对cat可能正确的情况进行处理
void outputCat(Node *root, string path, int &judge){
    //不管是就在根目录下的文件还是指定目录下的文件，一律按绝对路径处理
    if(path[0] != '/'){
        path = "/" + path;
    }
    if(path[path.length() - 1] != '/'){
        path = path + "/";
    }
    if(path == root->path){
        if(root->isfile){
            judge = 1;
            if(root->content[0] != '\0'){
                myPrint(root->content);
                myPrint("\n");
            }
            return;
        }else{
            judge = 2;
            myPrint("NO a File!\n");
            return;
        }
    }
    if (path.length() <= root->path.length()) {
        return;
    }
    string temp = path.substr(0, root->path.length());
    if (temp == root->path) {
        //路径部分匹配
        //处理路径中的.和..
        if(path[root->path.length()] == '.' && path[root->path.length() + 1] == '/'){
            string temp1 = path.substr(root->path.length()+2);
            outputCat(root, temp + temp1, judge);
        }else if(path[root->path.length()] == '.' && path[root->path.length() + 1] == '.'){
            string temp2 = path.substr(root->path.length()+3);
            outputCat(root->front, root->front->path + temp2, judge);
        }else{
            for (Node *q : root->next) {
                outputCat(q, path, judge);
            }
        }
    }
}

//处理ls指令
void HandleLs(vector<string> commands, Node *root){
    if(commands.size() == 1){//不添加任何选项且就在根目录
        outputLS(root);
    }else{
        bool hasL = false;//是否有可选项l，并且判断是否是有效的可选项
        // bool hasDir = false;//是否存在有效路径，路径个数是否为0或1
        string path = "/";//默认路径为根目录
        int cnt = 0;//记录路径参数个数
        for(int i = 1; i < commands.size(); i++){
            //TODO
            //指令中存在正确形式的-l可选项
            if(commands[i][0] == '-'){
                if(!isL(commands[i])){
                    myPrint("invalid option!\n");
                    return;
                }else{
                    hasL = true;
                }
            }else{
                cnt++;
                if(cnt >= 2){
                    myPrint("Too much to deal with!\n");
                    return;
                }else{
                    path = commands[i];
                }
            }
        }
        //至此已经得到是否有-l可选项以及路径
        int judge = 0;
        if(!hasL){
            outputLS(root, path, judge);
        }else{
            outputLSL(root, path, judge);
        }
        if(judge == 0){
            myPrint("No such directory!\n");
        }
    }
}

//从父目录输出文件目录结构
void outputLS(Node *root){
    Node *ptr = root;
    if(ptr->isfile){
        return;
    }else{//目录
        string str;
        str = ptr->path + ":\n";
        myPrint(str.c_str());
        Node *node;
        for(int i = 0; i < ptr->next.size(); i++){
            node = ptr->next[i];
            if(!node->isfile){
                str = "\033[31m" + node->name + "\033[0m";
            }else{
                str = node->name;
            }
            myPrint(str.c_str());
            myPrint(" ");
        }
        myPrint("\n");
        for(int i = 0; i < ptr->next.size(); i++){
            if(!ptr->next[i]->isfile && ptr->next[i]->isval){
                outputLS(ptr->next[i]);//向下输出其他目录下结构
            }
        }
    }
}

//判断指令中存在正确形式的-l可选项
bool isL(string &command){
    for(int i = 1; i < command.size(); i++){
        if(command[i] != 'l'){
            return false;
        }
    }
    return true;
}

//先前的outputLS就显得多余了，不过也懒得改了
//从指定目录作为根节点不加可选项输出
//跟cat对路径的处理是很像的，可惜没有提炼出方法，应该可以简化不少
void outputLS(Node *root, string path, int &judge){
    if(path[0] != '/'){
        path = "/" + path;
    }
    if(path[path.length() - 1] != '/'){
        path = path + "/";
    }
    if(path == root->path){
        if(!root->isfile){
            judge = 1;
            outputLS(root);
            return;
        }else{
            judge = 2;
            myPrint("NO a directory!\n");
            return;
        }
    }
    if (path.length() <= root->path.length()) {
        return;
    }
    string temp = path.substr(0, root->path.length());
    if (temp == root->path) {
        //路径部分匹配
        //处理路径中的.和..
        if(path[root->path.length()] == '.' && path[root->path.length() + 1] == '/'){
            string temp1 = path.substr(root->path.length()+2);
            outputLS(root, temp + temp1, judge);
        }else if(path[root->path.length()] == '.' && path[root->path.length() + 1] == '.'){
            string temp2 = path.substr(root->path.length()+3);
            outputLS(root->front, root->front->path + temp2, judge);
        }else{
            for (Node *q : root->next) {
                outputLS(q, path, judge);
            }
        }
    }
}

//加可选项输出
void outputLSL(Node *root){
    Node *ptr = root;
    if(ptr->isfile){
        return;
    }else{//目录
        string str;
        str = ptr->path + " " + to_string(ptr->dir_count) + " " + to_string(ptr->file_count) + ":\n";
        myPrint(str.c_str());
        Node *node;
        for(int i = 0; i < ptr->next.size(); i++){
            node = ptr->next[i];
            if(!node->isfile){
                if(node->isval){
                    str = "\033[31m" + node->name + "\033[0m";
                    str = str + " " + to_string(node->dir_count) + " " + to_string(node->file_count) + "\n";
                }else{
                    str = "\033[31m" + node->name + "\033[0m" + "\n";
                }
                
            }else{
                str = node->name;
                str = str + " " + to_string(node->FileSize) + "\n";
            }
            myPrint(str.c_str());
        }
        myPrint("\n");
        for(int i = 0; i < ptr->next.size(); i++){
            if(!ptr->next[i]->isfile && ptr->next[i]->isval){
                outputLSL(ptr->next[i]);//向下输出其他目录下结构
            }
        }
    }
}


//从指定目录作为根节点加可选项输出
void outputLSL(Node *root, string path, int &judge){
    if(path[0] != '/'){
        path = "/" + path;
    }
    if(path[path.length() - 1] != '/'){
        path = path + "/";
    }
    if(path == root->path){
        if(!root->isfile){
            judge = 1;
            outputLSL(root);
            return;
        }else{
            judge = 2;
            myPrint("NO a directory!\n");
            return;
        }
    }
    if (path.length() <= root->path.length()) {
        return;
    }
    string temp = path.substr(0, root->path.length());
    if (temp == root->path) {
        //路径部分匹配
        //处理路径中的.和..
        if(path[root->path.length()] == '.' && path[root->path.length() + 1] == '/'){
            string temp1 = path.substr(root->path.length()+2);
            outputLSL(root, temp + temp1, judge);
        }else if(path[root->path.length()] == '.' && path[root->path.length() + 1] == '.'){
            string temp2 = path.substr(root->path.length()+3);
            outputLSL(root->front, root->front->path + temp2, judge);
        }else{
            for (Node *q : root->next) {
                outputLSL(q, path, judge);
            }
        }
    }
}

// ---------------------------------------------
// main()

int main()
{
    FILE *fat12;
    fat12 = fopen("a.img", "rb");//open the img
    BPB bpb;
    // read the BPB and store them in the struct
    fseek(fat12, 11, SEEK_SET);
    fread(&bpb, 1, 25, fat12);
    // 初始化
    BytsPerSec = bpb.BPB_BytsPerSec;
    SecPerClus = bpb.BPB_SecPerClus;
    RsvdSecCnt = bpb.BPB_RsvdSecCnt;
    NumFATs = bpb.BPB_NumFATs;
    RootEntCnt = bpb.BPB_RootEntCnt;
    FATSz = bpb.BPB_FATSz16;
    Node *root = new Node();//设立根节点
    root->name = ("");
    root->path = ("/");
    root->front = root;//根节点向上还是根节点
    ReadDir(fat12, root);

    while(1){
        myPrint("> ");
        string input;
        getline(cin, input);//read one line command
        if(input.length() == 0){
            //no input, just change a line
            continue;
        }
        vector<string> commands = split(input, ' ');
        if(commands[0] == "exit"){
            // exit the program
            myPrint("See you Again!\n");
            fclose(fat12);
            break;
        }else if(commands[0] == "cat"){
            // output the content of the file
            HandleCat(commands, root);
        }else if(commands[0] == "ls"){
            // ls the file and floder
            HandleLs(commands, root);
        }else{
            // bad command
            myPrint("Command not found!\n");
        }
    }
    return 0;
}