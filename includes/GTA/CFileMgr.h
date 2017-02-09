class CFileMgr
{
public:
	static inline FILE*  OpenFile(const char* path, const char* mode)
	{
		return fopen(path, mode);
	};
	static inline  int  CloseFile(FILE* stream)
	{
		return fclose(stream);
	};
	static inline bool  ReadLine(FILE* stream, char* str, int num)
	{
		return fgets(str, num, stream) != nullptr;
	};
	static inline size_t Read(FILE* stream, void* buf, size_t len)
	{
		return fread(buf, 1, len, stream);
	};
	static inline size_t Write(FILE* stream, const char* ptr, size_t len)
	{
		return fwrite(ptr, 1, len, stream);
	};
	static inline bool  Seek(FILE* stream, long pos, int from)
	{
		return fseek(stream, pos, from) != 0;
	};
	static inline const char* LoadLine(FILE* hFile)
	{
		static char		cLineBuffer[512];

		if (!CFileMgr::ReadLine(hFile, cLineBuffer, sizeof(cLineBuffer)))
			return nullptr;

		for (int i = 0; cLineBuffer[i]; ++i)
		{
			if (cLineBuffer[i] == '\n')
				cLineBuffer[i] = '\0';
			else if (cLineBuffer[i] < ' ' || cLineBuffer[i] == ',')
				cLineBuffer[i] = ' ';
		}

		const char* p = cLineBuffer;
		while (*p <= ' ')
		{
			if (!*p++)
				break;
		}
		return p;
	};
};