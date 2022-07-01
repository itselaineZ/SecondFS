#include "../include/User.h"
#include "../include/Inode.h"

extern User g_User;
extern FileManager g_FileManager;

User::User() {
    u_error = U_NOERROR;
    fileManager = &g_FileManager;
	u_dirp = "/";   //  系统调用参数（一般用于pathname）设置为根目录
    u_curdir = "/"; //  当前工作目录完整路径
    //fileManager->Open();
	u_cdir = fileManager->rootDirInode; //  指向当前目录的Inode
    u_pdir = NULL;  //  指向父目录的Inode
    memset(u_arg, 0, sizeof(u_arg));
}

User::~User() {
}


void User::Mkdir(string dirName) {
    if (!checkPathName(dirName)) {
        return;
    }
    u_arg[1] = Inode::IFDIR;
    fileManager->Creat();
    IsError();
}

void User::Ls() {
    u_ls.clear();
    fileManager->Ls();
    if (IsError()) {
        return;
    }
    cout << u_ls << endl;
}

void User::Cd(string dirName) {
    if (!checkPathName(dirName)) {
        return;
    }
    fileManager->ChDir();
    IsError();
}

void User::Create(string fileName, string mode) {
    if (!checkPathName(fileName)) {
        return;
    }
    int md = INodeMode(mode);
    if (md == 0) {
        cout << "this mode is undefined !  \n";
        return;
    }

    u_arg[1] = md;
    fileManager->Creat();
    IsError();
}

void User::Delete(string fileName) {
    if (!checkPathName(fileName)) {
        return;
    }
    fileManager->UnLink();
    IsError();
}

void User::Open(string fileName, string mode) {
    if (!checkPathName(fileName)) {
        return;
    }
    int md = FileMode(mode);
    if (md == 0) {
        cout << "this mode is undefined ! \n";
        return;
    }

    u_arg[1] = md;
    fileManager->Open();
    if (IsError())
        return;
    cout << "open success, return fd=" << u_ar0[EAX] << endl;
}

void User::Close(string sfd) {
    if (sfd.empty() || !isdigit(sfd.front())) {
        cout << "parameter fd can't be empty or be nonnumeric ! \n";
        return;
    }
    u_arg[0] = stoi(sfd);
    fileManager->Close();
    IsError();
}

void User::Seek(string sfd, string offset, string origin) {
    if (sfd.empty() || !isdigit(sfd.front())) {
        cout << "parameter fd can't be empty or be nonnumeric ! \n";
        return;
    }
    if (offset.empty()) {
        cout << "parameter offset can't be empty ! \n";
        return;
    }
    if (origin.empty() || !isdigit(origin.front())) {
        cout << "parameter origin can't be empty or be nonnumeric ! \n";
        return;
    }
    u_arg[0] = stoi(sfd);
    u_arg[1] = stoi(offset);
    u_arg[2] = stoi(origin);
    fileManager->Seek();
    IsError();
}

void User::Write(string sfd, string inFile, string size) {
    if (sfd.empty() || !isdigit(sfd.front())) {
        cout << "parameter fd can't be empty or be nonnumeric ! \n";
        return;
    }
    int fd = stoi(sfd);

    int usize;
    if (size.empty() || (usize = stoi(size)) < 0) {
        cout << "parameter size must be greater or equal than 0 ! \n";
        return;
    }

    char *buffer = new char[usize];
    fstream fin(inFile, ios::in | ios::binary);
    if (!fin) {
        cout << "file " << inFile << " open failed ! \n";
        return;
    }
    fin.read(buffer, usize);
    fin.close();
    //cout << "fd = " << fd << " inFile = " << inFile << " size = " << usize << "\n";
    u_arg[0] = fd;
    u_arg[1] = (long long)buffer;
    u_arg[2] = usize;
    fileManager->Write();

    if (IsError())
        return;
    cout << "write " << u_ar0[User::EAX] << "bytes success !" << endl;
    delete[]buffer;
}

void User::Read(string sfd, string outFile, string size) {
    if (sfd.empty() || !isdigit(sfd.front())) {
        cout << "parameter fd can't be empty or be nonnumeric ! \n";
        return;
    }
    int fd = stoi(sfd);

    int usize;
    if (size.empty() || !isdigit(size.front()) || (usize = stoi(size)) < 0) {
        cout << "parameter size is not right \n";
        return;
    }
    char *buffer = new char[usize];
    //cout << "fd = " << fd << " outFile = " << outFile << " size = " << size << "\n";
    u_arg[0] = fd;
    u_arg[1] = (long)(buffer);
    u_arg[2] = usize;
    fileManager->Read();
    if (IsError())
        return;

    cout << "read " << u_ar0[User::EAX] << " bytes success : \n" ;
    if (outFile.empty()) {
        for (unsigned int i = 0; i < u_ar0[User::EAX]; ++i) {
            cout << (char)buffer[i];
        }
        cout << " \n";
        return;
    }
    fstream fout(outFile, ios::out | ios::binary);
    if (!fout) {
        cout << "file " << outFile << " open failed ! \n";
        return;
    }
    fout.write(buffer, u_ar0[User::EAX]);
    fout.close();
    cout << "read to " << outFile << " done ! \n";
    delete[]buffer;
}

int User::INodeMode(string mode) {
    int md = 0;
    if (mode.find("-r") != string::npos) {
        md |= Inode::IREAD;
    }
    if (mode.find("-w") != string::npos) {
        md |= Inode::IWRITE;
    }
    if (mode.find("-rw") != string::npos) {
        md |= (Inode::IREAD | Inode::IWRITE);
    }
    return md;
}

int User::FileMode(string mode) {
    int md = 0;
    if (mode.find("-r") != string::npos) {
        md |= File::FREAD;
    }
    if (mode.find("-w") != string::npos) {
        md |= File::FWRITE;
    }
    if (mode.find("-rw") != string::npos) {
        md |= (File::FREAD | File::FWRITE);
    }
    return md;
}

bool User::checkPathName(string path) {
    // FileManager 中函数不判断参数的合法性，最好在User中过滤，
    // 暂不考虑过多的参数不合法情况
    if (path.empty()) {
        cout << "parameter path can't be empty ! \n";
        return false;
    }

    //  返回上级目录
    if (path.substr(0, 2) != "..")
        u_dirp = path;
    else {
        string pre = u_curdir;
        unsigned int p = 0;
        //可以多重相对路径 .. ../ ../.. ../../
        for (; pre.length() > 3 && p < path.length() && path.substr(p, 2) == ".."; ) {
            pre.pop_back();
            pre.erase(pre.find_last_of('/') + 1);
            p += 2;
            p += p < path.length() && path[p] == '/';
        }
        u_dirp = pre + path.substr(p);
    }

    if (u_dirp.length() > 1 && u_dirp.back() == '/')
        u_dirp.pop_back();

    for (unsigned int p = 0, q = 0; p < u_dirp.length(); p = q + 1) {
        q = u_dirp.find('/', p);
        q = min(q, (unsigned int)u_dirp.length());
        if (q - p > DirectoryEntry::DIRSIZ) {
            cout << "the fileName or dirPath can't be greater than 28 size ! \n";
            return false;
        }
    }
    return true;
}

bool User::IsError() {
    if (u_error != U_NOERROR) {
        cout << "errno = " << u_error;
        EchoError(u_error);
        u_error = U_NOERROR;
        return true;
    }
    return false;
}

void User::EchoError(enum ErrorCode err) {
    string estr;
    switch (err) {
    case User::U_NOERROR:
        estr = " No u_error ";
        break;
    case User::U_ENOENT:
        estr = " No such file or directory ";
        break;
    case User::U_EBADF:
        estr = " Bad file number ";
        break;
    case User::U_EACCES:
        estr = " Permission denied ";
        break;
    case User::U_ENOTDIR:
        estr = " Not a directory ";
        break;
    case User::U_ENFILE:
        estr = " File table overflow ";
        break;
    case User::U_EMFILE:
        estr = " Too many open files ";
        break;
    case User::U_EFBIG:
        estr = " File too large ";
        break;
    case User::U_ENOSPC:
        estr = " No space left on device ";
        break;
    default:
        break;
    }
    cout << estr << endl;
}