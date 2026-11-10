#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <bcrypt.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <vector>
#include <map>
#include <set>
#include "ServerCore.h"
#include "sqlite3.h"
#include "json.hpp"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "advapi32.lib")

using json = nlohmann::json;

#define SVC_NAME L"ChatServerSvc"
#define XOR_KEY "ChatLab2026"
#define DEF_PORT 5000
#define MAX_PACKET 65536

struct Client{
	SOCKET s;
	int userId = 0;
	std::string name;
	std::wstring ip;
	CRITICAL_SECTION sendCs;

	explicit Client(SOCKET x) : s(x) {
		InitializeCriticalSection(&sendCs);
	}
	~Client() {
		DeleteCriticalSection(&sendCs);
	}
};

static CRITICAL_SECTION g_cs;
static struct CsInit {
	CsInit() {
		InitializeCriticalSection(&g_cs);
	}
}g_csInit;

struct Lk {
	Lk(){
		EnterCriticalSection(&g_cs);
	}
	~Lk(){
		LeaveCriticalSection(&g_cs);
	}
};

static HWND g_hwnd = NULL;
static std::wstring g_dir;
static sqlite3* g_db = nullptr;
static SOCKET g_ls = INVALID_SOCKET;
static HANDLE g_hAcc = NULL;
static volatile LONG g_run = 0;
static volatile LONG g_thr = 0;

static std::map<std::string, Client*> g_online;
static std::set<Client*> g_clients;

std::wstring Utf8ToWide(const std::string& s) {
	if (s.empty())
		return L"";
	int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), NULL, 0);
	std::wstring w(n, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), &w[0], n);
	return w;
}

static std::string WideToUtf8(const std::wstring& w) {
	if (w.empty())
		return "";
	int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), NULL, 0, NULL, NULL);
	std::string s(n, '\0');
	WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), &s[0], n, NULL, NULL);
	return s;
}

static std::wstring FmtV(const wchar_t* fmt, va_list ap) {
	wchar_t buf[512];
	_vsnwprintf_s(buf, _countof(buf), _TRUNCATE, fmt, ap);
	return buf;
}

static std::wstring Fmt(const wchar_t* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	std::wstring s = FmtV(fmt, ap);
	va_end(ap);
	return s;
}

static void Log(const wchar_t* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	std::wstring body = FmtV(fmt, ap);
	va_end(ap);

	SYSTEMTIME t;
	GetLocalTime(&t);
	std::wstring line = Fmt(L"[%02d:%02d:%02d]", t.wHour, t.wMinute, t.wSecond) + body;

	HANDLE f = CreateFileW((g_dir + L"ChatServer.log").c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (f != INVALID_HANDLE_VALUE) {
		std::string u = WideToUtf8(line) + "\r\n";
		DWORD written = 0;
		WriteFile(f, u.data(), (DWORD)u.size(), &written, NULL);
		CloseHandle(f);
	}

	if (g_hwnd) {
		wchar_t* copy = _wcsdup(line.c_str());
		if (copy && !PostMessageW(g_hwnd, WM_APP_LOG, 0, (LPARAM)copy))
			free(copy);
	}
}

static void PostUsers() {
	if (g_hwnd)
		PostMessageW(g_hwnd, WM_APP_USERS, 0, 0);
}

static std::string ToHex(const unsigned char* d, int n) {
	static const char* digits = "0123456789abcdef";
	std::string s;
	for (int i = 0; i < n; ++i) {
		s += digits[d[i] >> 4];
		s += digits[d[i] & 15];
	}
	return s;
}

static std::string GenSalt() {
	unsigned char b[16] = {};
	if (BCryptGenRandom(NULL, b, sizeof(b), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0)
		return "";
	return ToHex(b, sizeof(b));
}

static std::string HashPassword(const std::string& pass, const std::string& salt) {
	std::string input = salt + pass;
	BCRYPT_ALG_HANDLE alg = nullptr;
	unsigned char out[32] = {};
	if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, NULL, 0) != 0)
		return "";
	NTSTATUS st = BCryptHash(alg, NULL, 0, (PUCHAR)input.data(), (ULONG)input.size(), out, sizeof(out));
	BCryptCloseAlgorithmProvider(alg, 0);
	return (st == 0) ? ToHex(out, sizeof(out)) : "";
}

static bool IsValidUsername(const std::string& u) {
	if (u.empty() || u.size() > 80)
		return false;

	int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, u.data(), (int)u.size(), NULL, 0);
	if (n < 3 || n > 20)
		return false;
	std::wstring w(n, L'\0');
	MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, u.data(), (int)u.size(), &w[0], n);

	std::vector<WORD> type(n);
	if (!GetStringTypeW(CT_CTYPE1, w.c_str(), n, type.data()))
		return false;
	for (int i = 0; i < n; ++i){
		bool ok = (type[i] & (C1_ALPHA | C1_DIGIT)) != 0 || w[i] == L'_';
		if (!ok)
			return false;
	}
	return true;
}

static bool IsValidPassword(const std::string& p) {
	return p.size() >= 4 && p.size() <= 64;
}

static void XorBuf(std::string& data) {
	const char* key = XOR_KEY;
	size_t keyLen = strlen(key);
	for (size_t i = 0; i < data.size(); ++i)
		data[i] ^= key[i % keyLen];
}

static bool RecvAll(SOCKET s, char* buf, int len) {
	int got = 0;
	while (got < len) {
		int r = recv(s, buf + got, len - got, 0);
		if (r <= 0)
			return false;
		got += r;
	}
	return true;
}

static bool SendAll(SOCKET s, const char* buf, int len) {
	int sent = 0;
	while (sent < len) {
		int r = send(s, buf + sent, len - sent, 0);
		if (r == SOCKET_ERROR)
			return false;
		sent += r;
	}
	return true;
}

static bool RecvPacket(SOCKET s, std::string& out) {
	unsigned long n = 0;
	if (!RecvAll(s, (char*)&n, 4))
		return false;
	n = ntohl(n);

	if (n == 0 || n > MAX_PACKET)
		return false;
	
	out.assign(n, '\0');
	if (!RecvAll(s, &out[0], (int)n))
		return false;
	XorBuf(out);
	return true;
}

static bool SendPacket(Client* c, const json& j) {
	std::string data = j.dump(-1, ' ', false, json::error_handler_t::replace);
	XorBuf(data);
	unsigned long n = htonl((unsigned long)data.size());
	EnterCriticalSection(&c->sendCs);
	bool ok = SendAll(c->s, (const char*)&n, 4) && SendAll(c->s, data.data(), (int)data.size());
	LeaveCriticalSection(&c->sendCs);
	return ok;
}

struct Stmt {
	sqlite3_stmt* s = nullptr;
	explicit Stmt(const char* sql) {
		if (!g_db || sqlite3_prepare_v2(g_db, sql, -1, &s, nullptr) != SQLITE_OK)
			s = nullptr;
	}

	~Stmt() {
		if (s) sqlite3_finalize(s);
	}

	Stmt(const Stmt&) = delete;
	Stmt& operator=(const Stmt&) = delete;

	bool ok() const {
		return s != nullptr;
	}

	void BindText(int i, const std::string& v) {
		sqlite3_bind_text(s, i, v.c_str(), (int)v.size(), SQLITE_TRANSIENT);
	}

	void BindInt(int i, long long v) {
		sqlite3_bind_int64(s, i, v);
	}

	int Step() {
		return sqlite3_step(s);
	}
	std::string  ColText(int i) {
		const unsigned char* t = sqlite3_column_text(s, i);
		return t ? std::string((const char*)t) : std::string();
	}
	long long ColInt(int i) {
		return sqlite3_column_int64(s, i);
	}
};

static bool OpenDb() {
	std::string path = WideToUtf8(g_dir + L"chat.db");
	if (sqlite3_open(path.c_str(), &g_db) != SQLITE_OK) {
		g_db = nullptr;
		return false;
	}

	const char* sqlUsers = "PRAGMA foreign_keys = ON;"
		"CREATE TABLE IF NOT EXISTS Users ("
		" id INTEGER PRIMARY KEY AUTOINCREMENT,"
		" username TEXT NOT NULL,"
		" password_hash TEXT NOT NULL,"
		" salt TEXT NOT NULL,"
		" created_at INTEGER NOT NULL);";
	char* err = nullptr;
	if (sqlite3_exec(g_db, sqlUsers, nullptr, nullptr, &err) != SQLITE_OK) {
		sqlite3_free(err);
		return false;
	}

	const char* sqlMessages =
		"CREATE TABLE IF NOT EXISTS Messages ("
		" id INTEGER PRIMARY KEY AUTOINCREMENT,"
		" sender_id INTEGER NOT NULL REFERENCES Users(id),"
		" recv_id INTEGER NOT NULL REFERENCES Users(id),"
		" content TEXT NOT NULL,"
		" timestamp INTEGER NOT NULL);"
		"CREATE INDEX IF NOT EXISTS idx_msg_pair ON Messages(sender_id, recv_id, id);";
	if (sqlite3_exec(g_db, sqlMessages, nullptr, nullptr, &err) != SQLITE_OK) {
		sqlite3_free(err);
		return false;
	}
}

static void CloseDb() {
	Lk lock;
	if (g_db) {
		sqlite3_close(g_db);
		g_db = nullptr;
	}
}

static int DoRegister(const std::string& user, const std::string& pass) {
	if (!IsValidUsername(user) || !IsValidPassword(pass))
		return 2;

	std::string salt = GenSalt();
	std::string hash = salt.empty() ? "" : HashPassword(pass, salt);
	if (hash.empty())
		return 3;

	Lk lock;
	if (!g_db)
		return 3;

	{
		Stmt check("SELECT 1 FROM Users WHERE username = ?1");
		if (!check.ok())
			return 3;
		check.BindText(1, user);
		if (check.Step() == SQLITE_ROW)
			return 1;
	}

	Stmt ins("INSERT INTO Users(username, password_hash, salt, created_at) VALUES(?1, ?2, ?3, ?4)");
	if (!ins.ok())
		return 3;
	ins.BindText(1, user);
	ins.BindText(2, hash);
	ins.BindText(3, salt);
	ins.BindInt(4, (long long)time(NULL));
	int rc = ins.Step();
	if (rc == SQLITE_DONE)
		return 0;
	if (rc == SQLITE_CONSTRAINT)
		return 1;
	return 3;
}

static int DoLogin(const std::string& user, const std::string& pass, int& userId) {
	if (!IsValidUsername(user) || pass.empty() || pass.size() > 64)
		return 1;

	std::string dbHash, salt;

	{
		Lk lock;
		if (!g_db)
			return 3;
		Stmt st("SELECT id, password_hash, salt FROM Users WHERE username = ?1");
		if (!st.ok())
			return 3;
		st.BindText(1, user);

		if (st.Step() != SQLITE_ROW)
			return 1;
		userId = (int)st.ColInt(0);
		dbHash = st.ColText(1);
		salt = st.ColText(2);
	}

	std::string hash = HashPassword(pass, salt);
	if (hash.empty())
		return 3;
	return (hash == dbHash) ? 0 : 1;
}

static int DoSendMessage(int fromId, const std::string& to, const std::string& text, long long& ts, long long& msgId){
	if (text.empty() || text.size() > 2000)
		return 2;
	Lk lock;
	if (!g_db){
		Log(L"DoSendMessage: chua mo duoc database");
		return 3;
	}

	long long toId = 0;
	{
		Stmt st("SELECT id FROM Users WHERE username = ?1");
		if (!st.ok()){
			Log(L"DoSendMessage: loi SELECT: %s", Utf8ToWide(sqlite3_errmsg(g_db)).c_str());
			return 3;
		}
		st.BindText(1, to);
		if (st.Step() != SQLITE_ROW)
			return 1;
		toId = st.ColInt(0);
	}
	ts = (long long)time(NULL);
	Stmt ins("INSERT INTO Messages(sender_id, recv_id, content, timestamp) VALUES(?1, ?2, ?3, ?4)");
	if (!ins.ok()){
		Log(L"DoSendMessage: loi chuan bi INSERT: %s", Utf8ToWide(sqlite3_errmsg(g_db)).c_str());
		return 3;
	}
	ins.BindInt(1, fromId); ins.BindInt(2, toId); ins.BindText(3, text); ins.BindInt(4, ts);
	int rc = ins.Step();
	if (rc != SQLITE_DONE){
		Log(L"DoSendMessage: loi INSERT (ma %d): %s", rc, Utf8ToWide(sqlite3_errmsg(g_db)).c_str());
		return 3;
	}
	msgId = sqlite3_last_insert_rowid(g_db);
	return 0;
}

static int DoGetMessages(int myId, const std::string& with, long long sinceId, json& arr) {
	arr = json::array();
	std::vector<json> rows;
	{
		Lk lock;
		if (!g_db)
			return 3;
		long long otherId = 0;
		{
			Stmt st("SELECT id FROM Users WHERE username = ?1");
			if (!st.ok())
				return 3;
			st.BindText(1, with);
			if (st.Step() != SQLITE_ROW)
				return 1;
			otherId = st.ColInt(0);
		}
		Stmt q("SELECT m.id, s.username, r.username, m.content, m.timestamp "
			"FROM Messages m "
			"JOIN Users s ON s.id = m.sender_id "
			"JOIN Users r ON r.id = m.recv_id "
			"WHERE ((m.sender_id = ?1 AND m.recv_id = ?2) OR (m.sender_id = ?2 AND m.recv_id = ?1)) "
			"AND m.id > ?3 ORDER BY m.id DESC LIMIT 100");
		if (!q.ok())
			return 3;
		q.BindInt(1, myId); q.BindInt(2, otherId); q.BindInt(3, sinceId);
		while (q.Step() == SQLITE_ROW){
			json row = { {"id", q.ColInt(0)}, {"from", q.ColText(1)}, {"to", q.ColText(2)}, {"content", q.ColText(3)}, {"timestamp", q.ColInt(4)} };
			rows.push_back(row);
		}
	}
	for (size_t i = rows.size(); i-- > 0;)
		arr.push_back(rows[i]);
	return 0;
}

static json DoGetUsers(int myId){
	std::vector<std::string> names;
	{
		Lk lock;
		Stmt st("SELECT username FROM Users WHERE id <> ?1 ORDER BY username");
		if (st.ok()) {
			st.BindInt(1, myId);
			while (st.Step() == SQLITE_ROW)
				names.push_back(st.ColText(0));
		}
	}
	json arr = json::array();
	Lk lock;
	for (auto& n : names) {
		json u = {{"username", n}, {"online", g_online.count(n) > 0}};
		arr.push_back(u);
	}
	return arr;
}

static bool SetOnline(const std::string& user, Client* c){
	Lk lock;
	if (g_online.count(user))
		return false;
	g_online[user] = c;
	return true;
}

static void Offline(Client* c){
	if (!c->userId)
		return;
	{
		Lk lock;
		auto it = g_online.find(c->name);
		if (it != g_online.end() && it->second == c)
			g_online.erase(it);
	}
	Log(L"%s da dang xuat / thoat", Utf8ToWide(c->name).c_str());
	c->userId = 0;
	c->name.clear();
	PostUsers();
}

std::vector<std::string> OnlineNames(){
	Lk lock;
	std::vector<std::string> v;
	for (auto& p : g_online)
		v.push_back(p.first);
	return v;
}

static json Reply(const std::string& action, int code, const char* msg){
	json r = { {"action", action}, {"status", code == 0 ? "ok" : "error"}, {"code", code}, {"msg", msg} };
	return r;
}

static std::string JsonStr(const json& j, const char* key){
	auto it = j.find(key);
	return (it != j.end() && it->is_string()) ? it->get<std::string>() : std::string();
}

static long long JsonInt(const json& j, const char* key){
	auto it = j.find(key);
	return (it != j.end() && it->is_number_integer()) ? it->get<long long>() : 0;
}

static json HandleRequest(Client* c, const std::string& data) {
	json req = json::parse(data, nullptr, false);
	if (req.is_discarded() || !req.is_object())
		return Reply("", 9, "Goi tin khong hop le");

	std::string action = JsonStr(req, "action");
	std::string user = JsonStr(req, "username");
	std::string pass = JsonStr(req, "password");
	Log(L"%s -> %s", c->ip.c_str(), Utf8ToWide(action.substr(0, 30)).c_str());

	if (action == "register"){
		static const char* msg[] = {
			"Dang ky thanh cong",
			"Tai khoan da ton tai",
			"Ten dang nhap (3-20 ky tu: chu, so, _) hoac mat khau (4-64 ky tu) khong hop le",
			"Loi may chu" };
		int rc = DoRegister(user, pass);
		return Reply(action, rc, msg[rc]);
	}

	if (action == "login"){
		if (c->userId)
			return Reply(action, 2, "Ban da dang nhap roi");

		int id = 0;
		int rc = DoLogin(user, pass, id);
		if (rc == 1)
			return Reply(action, 1, "Sai tai khoan hoac mat khau");
		if (rc != 0)
			return Reply(action, 3, "Loi may chu");

		if (!SetOnline(user, c))
			return Reply(action, 2, "Tai khoan dang dang nhap o noi khac");

		c->userId = id;
		c->name = user;
		Log(L"%s da dang nhap", Utf8ToWide(user).c_str());
		PostUsers();
		return Reply(action, 0, "Dang nhap thanh cong");
	}


	if (!c->userId)
		return Reply(action, 5, "Chua dang nhap");

	if (action == "logout"){
		Offline(c);
		return Reply(action, 0, "Da dang xuat");
	}

	if (action == "get_friends"){
		json r = Reply(action, 0, "OK");
		r["users"] = DoGetUsers(c->userId);
		return r;
	}

	if (action == "send_message"){
		static const char* msg[] = {"Da gui", "Nguoi nhan khong ton tai", "Noi dung rong hoac qua dai (toi da 2000 byte)", "Loi may chu"};
		std::string to = JsonStr(req, "to"), text = JsonStr(req, "content");
		long long ts = 0, msgId = 0;
		int rc = DoSendMessage(c->userId, to, text, ts, msgId);
		json r = Reply(action, rc, msg[rc]);
		if (rc == 0){
			r["id"] = msgId; r["timestamp"] = ts;
			json push = {{"action", "new_message"}, {"id", msgId}, {"from", c->name}, {"to", to}, {"content", text}, {"timestamp", ts}};
			Lk lock;
			auto it = g_online.find(to);
			if (it != g_online.end() && it->second != c)
				SendPacket(it->second, push);
		}
		return r;
	}

	if (action == "get_messages"){
		static const char* msg[] = {"OK", "Nguoi dung khong ton tai", "", "Loi may chu"};
		json arr;
		int rc = DoGetMessages(c->userId, JsonStr(req, "with"), JsonInt(req, "since_id"), arr);
		json r = Reply(action, rc, msg[rc]);
		if (rc == 0) r["messages"] = arr;
		return r;
	}

	return Reply(action, 9, "Hanh dong khong duoc ho tro");
}

static DWORD WINAPI ClientThread(LPVOID param){
	Client* c = (Client*)param;
	std::string data;

	while (RecvPacket(c->s, data)){
		json res;
		try{
			res = HandleRequest(c, data);
		}catch (...){
			res = Reply("", 3, "Du lieu sai dinh dang");
		}
		if (!SendPacket(c, res))
			break;
	}

	Offline(c);
	Log(L"%s ngat ket noi", c->ip.c_str());
	{
		Lk lock;
		g_clients.erase(c);
	}
	closesocket(c->s);
	delete c;
	InterlockedDecrement(&g_thr);
	return 0;
}

static DWORD WINAPI AcceptThread(LPVOID){
	SOCKET ls = g_ls;
	while (g_run){
		sockaddr_in ca = {};
		int len = sizeof(ca);
		SOCKET s = accept(ls, (sockaddr*)&ca, &len);
		if (s == INVALID_SOCKET){
			if (!g_run) break;
			Sleep(50);
			continue;
		}

		Client* c = new Client(s);
		wchar_t ip[46] = {};
		InetNtopW(AF_INET, &ca.sin_addr, ip, _countof(ip));
		c->ip = ip;
		Log(L"%s ket noi", ip);

		{
			Lk lock;
			g_clients.insert(c);
		}
		InterlockedIncrement(&g_thr);
		HANDLE h = CreateThread(NULL, 0, ClientThread, c, 0, NULL);
		if (h){
			CloseHandle(h);
		}
		else{
			{
				Lk lock;
				g_clients.erase(c);
			}
			closesocket(s);
			delete c;
			InterlockedDecrement(&g_thr);
		}
	}
	return 0;
}

void InitExeDir(){
	wchar_t p[MAX_PATH] = {};
	GetModuleFileNameW(NULL, p, MAX_PATH);
	g_dir = p;
	g_dir.resize(g_dir.find_last_of(L'\\') + 1);
}

void SetLogWindow(HWND h){
	g_hwnd = h;
}
bool ServerIsRunning(){
	return g_run != 0;
}

int ReadPortFromIni(){
	return (int)GetPrivateProfileIntW(L"Server", L"Port", DEF_PORT, (g_dir + L"ChatServer.ini").c_str());
}
void SavePortToIni(int port){
	WritePrivateProfileStringW(L"Server", L"Port", std::to_wstring(port).c_str(), (g_dir + L"ChatServer.ini").c_str());
}

bool ServerStart(int port, std::wstring& err){
	if (g_run) return true;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0){
		err = L"WSAStartup loi"; return false;
	}

	auto fail = [&](const wchar_t* what){
		err = Fmt(L"%s (ma loi %d)", what, WSAGetLastError());
		if (g_ls != INVALID_SOCKET){
			closesocket(g_ls); g_ls = INVALID_SOCKET;
		}
		CloseDb();
		WSACleanup();
		return false;
	};

	if (!OpenDb())
		return fail(L"Khong mo duoc chat.db");

	g_ls = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (g_ls == INVALID_SOCKET)
		return fail(L"socket loi");

	sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons((u_short)port);
	if (bind(g_ls, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
		return fail(L"bind loi (cong dang duoc chuong trinh khac su dung?)");

	if (listen(g_ls, SOMAXCONN) == SOCKET_ERROR)
		return fail(L"listen loi");

	InterlockedExchange(&g_run, 1);
	g_hAcc = CreateThread(NULL, 0, AcceptThread, NULL, 0, NULL);
	if (!g_hAcc){
		InterlockedExchange(&g_run, 0);
		return fail(L"CreateThread loi");
	}

	Log(L"Server dang lang nghe cong %d", port);
	return true;
}

void ServerStop() {
	if (!InterlockedExchange(&g_run, 0))
		return;

	closesocket(g_ls);
	g_ls = INVALID_SOCKET;
	WaitForSingleObject(g_hAcc, 5000);
	CloseHandle(g_hAcc);
	g_hAcc = NULL;

	{
		Lk lock;
		for (Client* c : g_clients)
			shutdown(c->s, SD_BOTH);
	}
	for (int i = 0; i < 100 && g_thr > 0; ++i)
		Sleep(50);

	CloseDb();
	WSACleanup();
	Log(L"Server da dung");
}

static SERVICE_STATUS_HANDLE g_ssh = NULL;
static HANDLE g_stopEvent = NULL;

static void SetServiceState(DWORD state){
	static DWORD checkPoint = 1;
	SERVICE_STATUS st = {};
	st.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	st.dwCurrentState = state;
	st.dwControlsAccepted = (state == SERVICE_RUNNING) ? (SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN) : 0;
	st.dwWin32ExitCode = NO_ERROR;
	st.dwCheckPoint = (state == SERVICE_RUNNING || state == SERVICE_STOPPED) ? 0 : checkPoint++;
	st.dwWaitHint = 10000;
	SetServiceStatus(g_ssh, &st);
}

static void WINAPI ServiceCtrlHandler(DWORD ctrl){
	if (ctrl == SERVICE_CONTROL_STOP || ctrl == SERVICE_CONTROL_SHUTDOWN){
		SetServiceState(SERVICE_STOP_PENDING);
		SetEvent(g_stopEvent);
	}
}

static void WINAPI ServiceMain(DWORD, LPWSTR*){
	g_ssh = RegisterServiceCtrlHandlerW(SVC_NAME, ServiceCtrlHandler);
	if (!g_ssh) return;

	g_stopEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
	if (!g_stopEvent){
		SetServiceState(SERVICE_STOPPED);
		return;
	}
	SetServiceState(SERVICE_START_PENDING);

	std::wstring err;
	if (!ServerStart(ReadPortFromIni(), err)){
		Log(L"Service khong khoi dong duoc: %s", err.c_str());
		SetServiceState(SERVICE_STOPPED);
		return;
	}
	SetServiceState(SERVICE_RUNNING);

	WaitForSingleObject(g_stopEvent, INFINITE);
	ServerStop();
	CloseHandle(g_stopEvent);
	SetServiceState(SERVICE_STOPPED);
}

void RunServiceDispatcher() {
	SERVICE_TABLE_ENTRYW table[] = {{(LPWSTR)SVC_NAME, ServiceMain}, {NULL, NULL}};
	StartServiceCtrlDispatcherW(table);
}

std::wstring InstallService(){
	wchar_t path[MAX_PATH] = {};
	GetModuleFileNameW(NULL, path, MAX_PATH);
	std::wstring cmd = L"\"" + std::wstring(path) + L"\" -service";

	SC_HANDLE scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	if (!scm)
		return Fmt(L"Khong mo duoc Service Manager (loi %lu). Hay chay bang quyen Administrator.", GetLastError());

	SC_HANDLE svc = CreateServiceW(scm, SVC_NAME, L"Chat Server (Buoi 6)", SERVICE_ALL_ACCESS,
		SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
		cmd.c_str(), NULL, NULL, NULL, NULL, NULL);
	DWORD e = GetLastError();
	if (svc)
		CloseServiceHandle(svc);
	CloseServiceHandle(scm);

	if
		(svc)
		return L"Da cai service ChatServerSvc (tu khoi dong cung Windows).\n" L"Bam Dung o cua so nay truoc, roi mo cmd (Administrator) go: sc start ChatServerSvc";
	if (e == ERROR_SERVICE_EXISTS)
		return L"Service da ton tai.";
	return Fmt(L"CreateService loi (ma loi %lu).", e);
}

// Gỡ service: dừng nếu đang chạy rồi xóa khỏi Windows
std::wstring UninstallService() {
	SC_HANDLE scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
	if (!scm)
		return Fmt(L"Khong mo duoc Service Manager (loi %lu). Hay chay bang quyen Administrator.", GetLastError());

	SC_HANDLE svc = OpenServiceW(scm, SVC_NAME, SERVICE_STOP | DELETE);
	std::wstring result = L"Khong tim thay service.";
	if (svc) {
		SERVICE_STATUS st;
		ControlService(svc, SERVICE_CONTROL_STOP, &st);
		result = DeleteService(svc) ? std::wstring(L"Da go service.")
			: Fmt(L"DeleteService loi (ma loi %lu).", GetLastError());
		CloseServiceHandle(svc);
	}
	CloseServiceHandle(scm);
	return result;
}
