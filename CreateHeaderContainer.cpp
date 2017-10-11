#include "CreateHeaderContainer.h"
#include "SocketSIMDIS.h"
#include <io.h>

#define O_RDWR        0x0002
#define O_CREAT        0x0100
#define O_TRUNC        0x0200

#define DCSDB_CODE "DCSD"
#define DCSDB_CODE_SIZE 4
#define DCS_VERSION 0x0204

#define MAX_DATAGRAM_SIZE 16384

struct DCSDBHeader
{
public:
	char code_[DCSDB_CODE_SIZE];
	unsigned short version_;
};

CreateHeaderContainer::CreateHeaderContainer()
: fd_(-1)
, save_(false)
{
	memset(filename_, 0, sizeof(filename_));
}


CreateHeaderContainer::~CreateHeaderContainer()
{
	clearItems();
	if (fd_ != -1)
	{
		close(fd_);
		fd_ = -1;
		if (!save_)
			unlink(filename_);
	}
}

void CreateHeaderContainer::clearItems()
{
	HeaderTable::const_iterator i;
	for (i = table_.begin(); i != table_.end(); ++i)
		delete i->second;
	table_.clear();
}

bool CreateHeaderContainer::Restore(const char* filename, unsigned short* version)
{
	DCSDBHeader idblock;
	int flags = O_CREAT | O_RDWR;

	//Add additional flags if they are available
//#ifdef O_BINARY
//	flags |= O_BINARY;
//#endif
//
//#ifdef O_RANDOM
//	flags |= O_RANDOM;
//#endif

	if (filename == NULL)
		return false;

	fd_ = open(filename, flags, 0644);

	if (fd_ != -1)
	{
		//Restore it
		DCSHeaderRecord* record = NULL;
		char* baseBuffer = NULL;
		DCSHeaderGroup headergroup;
		DCSFileEntryHeader tag;
		int location = 0;
		int result = 0;

		//Read id and version, and make sure they match.
		result = read(fd_, &idblock, sizeof(idblock));
		if (result != sizeof(idblock))
		{
			close(fd_);
			fd_ = -1;
			return false;
		}

		if (version != NULL) *version = idblock.version_;

		if ((memcmp(idblock.code_, DCSDB_CODE, DCSDB_CODE_SIZE) != 0) || (idblock.version_ != DCS_VERSION))
		{
			close(fd_);
			fd_ = -1;
			return false;
		}

		do
		{
			//Linux does not implement tell.
			//      location=tell(fd_);

			result = read(fd_, (void*)&tag, sizeof(tag));

			//Zero indicates eof
			if (result == 0)
				break;
			else if (result != sizeof(tag))
			{
				close(fd_);
				fd_ = -1;
				return false;
			}

			if (tag.active_)
			{
				baseBuffer = NULL;
				switch (tag.type_)
				{
				case DCSPLATFORMHEADER:
					if (tag.size_ == sizeof(headergroup.platformHeaderBuffer))
					{
						baseBuffer = headergroup.platformHeaderBuffer;
					}
					break;
				case DCSBEAMHEADER:
					if (tag.size_ == sizeof(headergroup.beamHeaderBuffer))
					{
						baseBuffer = headergroup.beamHeaderBuffer;
					}
					break;
				case DCSGATEHEADER:
					/*if (tag.size_ == sizeof(headergroup.gateHeaderBuffer))
					{
						baseBuffer = headergroup.gateHeaderBuffer;
					}*/
					break;
				default:
					break;
				}

				//Either the type was unknown or the size was wrong.  Don't read this file.
				if (baseBuffer == NULL)
				{
					clearItems();
					close(fd_);
					fd_ = -1;
					return false;
				}

				result = read(fd_, (void*)baseBuffer, tag.size_);

				//We don't check for end of file this time because an
				//end of file here indicates a corrupted/invalid file.
				if (result != tag.size_)
				{
					delete[] baseBuffer;
					clearItems();
					close(fd_);
					fd_ = -1;
					return false;
				}

				//printf("Header type: %d size is %d\n", header->getType(), header->sizeOf());

				record = new DCSHeaderRecord;
				record->size_ = sizeof(baseBuffer);
				record->location_ = location;


				DLPSBufferHandler baseHadler(baseBuffer);
				baseHadler.SetOffset(4);
				unsigned long long id =	DLPSReadLongLong(baseHadler);
				//         std::pair<uint32_t,DCSHeaderRecord*> entry(header->id_,record);
				std::pair<unsigned long long, DCSHeaderRecord*> entry(id, record);
				std::pair<HeaderTable::iterator, bool> p = table_.insert(entry);

				if (!p.second)
				{
					//Could not insert?
					clearItems();
					close(fd_);
					fd_ = -1;
					return false;
				}
			}
			else
			{
				lseek(fd_, tag.size_, SEEK_CUR);
			}

			//Mark location of the next record
			location += sizeof(tag)+tag.size_;
		} while (result != 0);

		strncpy(filename_, filename, sizeof(filename_)-1);
		return true;
	}

	return false;
}

bool CreateHeaderContainer::Initialize(const char* filename)
{
	DCSDBHeader idblock;
	int flags = O_CREAT | O_TRUNC | O_RDWR;
	if (filename == NULL)
	{
		return false;
	}

	fd_ = open(filename, flags, 0644);

	if (fd_ == -1)
	{
		return false;
	}

	//Write the file type followed by the version.
	memcpy(idblock.code_, DCSDB_CODE, DCSDB_CODE_SIZE);
	idblock.version_ = DCS_VERSION;
	if (write(fd_, &idblock, sizeof(idblock)) == -1)
	{
		close(fd_);
		fd_ = -1;
		return false;
	}

	strncpy(filename_, filename, strlen(filename) - 1);
	return true;
}

bool CreateHeaderContainer::Insert(DLPSBufferHandler& baseHeaderHandler, unsigned long long id)
//bool CreateHeaderContainer::Insert(const char* buffer, unsigned long long id)
{
	DCSHeaderRecord* record = new DCSHeaderRecord;

	//DLPSBufferHandler baseHeaderHandler(buffer);
	//baseHeaderHandler.SetOffset(4);
	//unsigned long long id = DLPSReadLongLong(baseHeaderHandler);

	record->size_ = baseHeaderHandler.GetByteOffset(); 	

	std::pair<unsigned long long, DCSHeaderRecord*> entry(id, record);
	std::pair<HeaderTable::iterator, bool> iter = table_.insert(entry);

	if (iter.second)
	{
		record->location_ = lseek(fd_, 0, SEEK_END);

		if (record->location_ != -1)
		{
			baseHeaderHandler.SetOffset(1);
			short baseType = DLPSReadShort(baseHeaderHandler);
			short baseTotalByteSize = baseHeaderHandler.GetByteOffset();   // Size가 3이 나옴.

			DCSFileEntryHeader tag;
			tag.active_ = 1;
			tag.type_ = baseType;
			tag.size_ = baseTotalByteSize;

			if (write(fd_, (const void*)&tag, sizeof(tag)) != -1)
			{
				if (write(fd_, (const void*)baseHeaderHandler.GetBuffer(), baseTotalByteSize) != -1)
				{
					return true;  
				}
				else
				{
					//clean();
				}
			}
		}
		else
		{
			table_.erase(id);
		}
	}

	delete record;
	return false;
}


bool CreateHeaderContainer::Update(DLPSBufferHandler& baseHeaderHandler, unsigned long long id, bool present)
//bool CreateHeaderContainer::Update(const char* buffer, unsigned long long id, bool present)
{
	//baseHeaderHandler.SetOffset(4);
	//unsigned long long id = DLPSReadLongLong(baseHeaderHandler);

	//DLPSBufferHandler handler(buffer);
	HeaderTable::iterator iter = table_.find(id);

	if (iter == table_.end())
	{
		present = false;
		return Insert(baseHeaderHandler, id);
	}

	present = true;

	//We overwrite the entry with this one.
	if (iter->second->size_ != baseHeaderHandler.GetByteOffset())
	{
		return false;  //Something is wrong.  Sizes don't match.
	}

	//Now we can put it in the file
	//  if(lseek(fd_,i->second->location_,SEEK_SET)==-1)
	int pos = lseek(fd_, iter->second->location_, SEEK_SET);
	if (pos == -1)
	{
		return false;
	}

	baseHeaderHandler.Skip(4);
	short baeType = DLPSReadShort(baseHeaderHandler);
	short baseTotalByteSize = baseHeaderHandler.GetByteOffset();

	//Update the file tag
	DCSFileEntryHeader tag;
	tag.active_ = 1;
	tag.type_ = baeType;
	tag.size_ = baseTotalByteSize;
	if (write(fd_, (const void*)&tag, sizeof(tag)) == -1)
	{
		return false;
	}

	if (write(fd_, (const void*)baseHeaderHandler.GetBuffer(), baseHeaderHandler.GetByteOffset()) == -1)
	{
		return false;
	}
	return true;
}

bool CreateHeaderContainer::Remove(unsigned long long id)
{
	HeaderTable::iterator iter = table_.find(id);

	if (iter == table_.end())
		return false;

	//Now we can update the file
	if (lseek(fd_, iter->second->location_, SEEK_SET) == -1)
		return false;

	//Update the file tag - mark for deletion
	DCSFileEntryHeader tag;
	tag.active_ = 0;
	tag.type_ = 0; // DCSUNKNOWN; Error: 재정의
	tag.size_ = iter->second->size_;
	if (write(fd_, (const void*)&tag, sizeof(tag)) == -1)
		return false;

	//Now Remove from table
	delete iter->second;
	table_.erase(iter);

	return true;
}

//SocketSIMDIS* CreateHeaderContainer::Retrieve(DLPSBufferHandler& baseHandler, unsigned long long id, DCSHeaderGroup* hg) const
//{
	//HeaderTable::const_iterator i = table_.find(id);

	////First clear the HeaderGroup so it has a type of 0 and baseheader_==NULL on failure.
	//if (hg)
	//{
	//	hg->type_ = 0;
	//	hg->baseheader_ = NULL;
	//}

	//if (i == table_.end())
	//	return NULL;

	////Now we can look in the file
	//if (lseek(fd_, i->second->location_, SEEK_SET) == -1)
	//	return NULL;

	////Get the file tag
	//DCSFileEntryHeader tag;
	//if (read(fd_, (void*)&tag, sizeof(tag)) != sizeof(tag))
	//	return NULL;

	//switch (tag.type_)
	//{
	//case DCSPLATFORMHEADER:
	//	header = (hg) ? &hg->platheader_ : new DCSPlatformHeader;
	//	break;
	//case DCSBEAMHEADER:
	//	header = (hg) ? &hg->beamheader_ : new DCSBeamHeader;
	//	break;
	//case DCSGATEHEADER:
	//	header = (hg) ? &hg->gateheader_ : new DCSGateHeader;
	//	break;
	//default:
	//	return NULL;
	//}

	//if (hg)
	//{
	//	hg->type_ = tag.type_;
	//	hg->baseheader_ = header;
	//}

	//if (read(fd_, (void*)header, header->sizeOf()) != header->sizeOf())
	//{
	//	if (hg == NULL) delete header;
	//	return NULL;
	//}

	//return header;
//}

//void CreateHeaderContainer::CreateFileEntryHeaer(DLPSBufferHandler& handler, char active, char pad, short type, short size)
//{
//	DLPSWriteChar(handler, active);
//	DLPSWriteChar(handler, pad);
//	DLPSWriteShort(handler, type);
//	DLPSWriteShort(handler, size);
//}
//
//void CreateHeaderContainer::CreateHeaderRecord(DLPSBufferHandler& handler, int location, unsigned short size/*baseTotalSize*/, short pad)
//{
//	DLPSWriteInteger(handler, location);
//	DLPSWriteShort(handler, size);
//	DLPSWriteShort(handler, pad);
//}
//
//void CreateHeaderContainer::CreateHeaderGroup(DLPSBufferHandler& handler, short type, DLPSBufferHandler& baseHdrHdlr, DLPSBufferHandler& platformHdrHdlr, DLPSBufferHandler& beamHdrHdlr)
//{
//	DLPSWriteShort(handler, type);
//}