#include "rcon_parse.h"

#include <thread>
#include <future>
#include <unistd.h>
#include <game/client/gameclient.h>
#include <game/client/components/console.h>

void CGameConsoleParse::OnInit() {
    RefreshClientsInfo();
}

inline void CGameConsoleParse::RefreshClientsInfo() {
    for(short i = 0; i < MAX_CLIENTS; i++)ClientsInfo[i] = ClientInfoPayload;
}

void CGameConsoleParse::RconAuthenticated(bool status) {
    m_rconAuthenticated = status;
}

void CGameConsoleParse::Refresh() {
    if(!m_rconAuthenticated)return;

    RefreshClientsInfo(); // отчищаем массив

    CGameConsole::CInstance *rconConsole = m_pClient->m_GameConsole.ConsoleForType(CGameConsole::CONSOLETYPE_REMOTE);
    CStaticRingBuffer<CGameConsole::CInstance::CBacklogEntry, 1024 * 1024, CRingBufferBase::FLAG_RECYCLE> rconConsoleLog = rconConsole->m_BacklogPending;

    rconConsole->ExecuteLine("show_ips 1");
    rconConsole->ExecuteLine("status");

    short processedCount = 200; // Counter for processed entries

    for (CGameConsole::CInstance::CBacklogEntry *pEntry = rconConsoleLog.Last(); pEntry; pEntry = rconConsoleLog.Prev(pEntry)) {
        if (processedCount < 0) break;
        processedCount--;

        if (!IsValidLogEntry(pEntry->m_aText)) continue; // Validate line
        ClientInfo pClient = ParseRconLine(pEntry->m_aText);
        
        if (strcmp(ClientsInfo[pClient.id].addr, ClientInfoPayload.addr) != 0) continue; // Убираем возможность перезаписи

        ClientsInfo[pClient.id] = pClient;
    }

}

ClientInfo CGameConsoleParse::GetClientById(short ClientId) {
    if(ClientId > MAX_CLIENTS || ClientId < 0)
    return ClientInfoPayload;
    return ClientsInfo[ClientId];
}

// Регулярное выражение для валидации строки
bool CGameConsoleParse::IsValidLogEntry(const char* text) { 
    std::regex logPattern(R"(^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2} I server: id=(?:[0-6]?\d|[1-9][0-9]) (?:\([a-f0-9-]+\) )?addr=[^ ]+ name='[^']*' (?:kog_id=\d+ )?client=\d+ secure=(yes|no) flags=([0-9]{1,3}|[1-9][0-9]{1,2}|1000)(?:\s+dnsbl=[a-zA-Z]+)?(?:\s+key=[^ ]+(?:\s+\([^)]+\))?)?$)");
    return std::regex_match(text, logPattern);
}

// Извлечения IP-адреса
std::string CGameConsoleParse::extractIP(const std::string& input) {
    size_t colonPos = input.find(':');
    if (colonPos != std::string::npos) return input.substr(0, colonPos); // Возвращаем часть строки до двоеточия
    return input;
}
// Регулярное выражение для парсинга строк
ClientInfo CGameConsoleParse::ParseRconLine(const char* line) {
    std::regex Regex(R"((\w+)=('[^']*'|[^ ]+))");
    std::smatch match;

    std::string LineStr(line);

    ClientInfo Result = ClientInfoPayload;

    // Итерация по всем совпадениям
    auto it = LineStr.cbegin();
    while (std::regex_search(it, LineStr.cend(), match, Regex)) {
        std::string key = match[1].str(); // Имя параметра
        std::string value = match[2].str(); // Значение параметра

        // Удаляем одинарные кавычки из значений, если они есть
        if (value.front() == '\'' && value.back() == '\'') value = value.substr(1, value.size() - 2);

        if(key=="id")Result.id = atoi(value.c_str());
        else if(key=="addr")strcpy(Result.addr, extractIP(value).c_str());
        else if(key=="name")strcpy(Result.name, value.c_str());
        else if(key=="kog_id")Result.kog_id = atoi(value.c_str());
        else if(key=="client")Result.client = atoi(value.c_str());
        else if(key=="secure"){
            if(value=="yes")Result.secure = true;
            else Result.secure = false;
        }
        else if(key=="flags")Result.flags = atoi(value.c_str());
        else if(key=="dnsbl")strcpy(Result.dnsbl, value.c_str());
        else if(key=="key")Result.key = value;
        
        it = match[0].second;
    }
    return Result;
}