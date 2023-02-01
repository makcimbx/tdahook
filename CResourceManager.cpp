#define _CRT_SECURE_NO_WARNINGS
#include "CResourceManager.h"

CResourceManager::CResourceManager()
{
	m_nextTextId = 1;
	m_nextUIId = 1;
	m_modified = false;
}

CResourceManager::~CResourceManager()
{
	for (std::map<UINT32, PackageText*>::iterator it = m_textDatabase.begin(); it != m_textDatabase.end(); it++)
	{
		if (it->second->sourceLen > 0 && it->second->source != NULL) delete[] it->second->source;
		if (it->second->translationLen > 0 && it->second->translation != NULL) delete[] it->second->translation;

		delete it->second;
	}

	for (std::map<UINT32, PackageText*>::iterator it = m_uiDatabase.begin(); it != m_uiDatabase.end(); it++)
	{
		if (it->second->sourceLen > 0 && it->second->source != NULL) delete[] it->second->source;
		if (it->second->translationLen > 0 && it->second->translation != NULL) delete[] it->second->translation;

		delete it->second;
	}

	for (ImageMap::iterator it = m_imageDatabase.begin(); it != m_imageDatabase.end(); it++)
	{
		if (it->second->replacementLen > 0) delete[] it->second->replacement;
		if (it->second->titleLen > 0) delete[] it->second->title;

		delete[] it->second->hash; // it->second->hash = it->first
		delete it->second;
	}

	for (TextLookupMap::iterator it = m_fastTextLookup.begin(); it != m_fastTextLookup.end(); it++)
	{
		// PackageText objects were deleted in first loop
		it->second->clear();

		delete it->second;
	}

	m_textDatabase.clear();
	m_uiDatabase.clear();
	m_imageDatabase.clear();
	m_fastTextLookup.clear();
}

bool CResourceManager::TranslateText(const char* original, char* buffer, int bufferSize)
{
	UINT32 originalLen = strlen(original);

	TextLookupMap::const_iterator mp = m_fastTextLookup.find(originalLen);

	if (mp != m_fastTextLookup.end())
	{
		for (std::vector<PackageText*>::const_iterator it = mp->second->begin(); it != mp->second->end(); it++)
		{
			if (strcmp((*it)->source, original) == 0)
			{
				if ((*it)->translationLen > 0)
				{
					strncpy(buffer, (*it)->translation, bufferSize - 1);
					buffer[bufferSize - 1] = '\x00';
				
					return true;
				}

				return false;
			}
		}
	}

	m_logger->WriteLine("New dialogue line found: ").WriteText(original);

	PackageText* data = new PackageText;
	data->sourceLen = originalLen;
	data->translationLen = 0;
	data->source = new char[data->sourceLen + 1];
	data->translation = NULL;
	strncpy(data->source, original, (data->sourceLen + 1));

	m_textDatabase.insert(std::pair<UINT32, PackageText*>(m_nextTextId, data));
	m_nextTextId++;
	m_modified = true;

	if (mp != m_fastTextLookup.end())
	{
		mp->second->push_back(data);
	}
	else
	{
		std::vector<PackageText*>* vec = new std::vector<PackageText*>;

		vec->push_back(data);
		
		auto result = std::pair<int, std::vector<PackageText*>*>(originalLen, vec);

		m_fastTextLookup.emplace(result);
	}

	return false;
}

// Loading/saving below
void CResourceManager::LoadPackage(const char* filename)
{
	int size = MultiByteToWideChar(CP_UTF8, NULL, filename, -1, NULL, 0);
	wchar_t* widefile = new wchar_t[size];
	MultiByteToWideChar(CP_UTF8, NULL, filename, -1, widefile, size);

	this->LoadPackage(widefile);

	delete[] widefile;
}

void CResourceManager::LoadPackage(const std::wstring& filename)
{
	std::ifstream loadingPackage(filename, std::wios::in | std::wios::binary);

	if (!loadingPackage)
	{
		m_logger->WriteLine("Unable to open package file");
		return;
	}

	// Try to load header
	PackageHeader header;

	loadingPackage.read((char*) &header, sizeof(PackageHeader));

	if (header.magicHeader != PACKAGE_MAGIC_HEADER)
	{
		m_logger->WriteLine("Package file has invalid header");
		loadingPackage.close();

		return;
	}

	// Text translations
	loadingPackage.seekg(header.textOffset);
	
	for (UINT32 i = 0; i < header.textCount; i++)
	{
		UINT32 id;
		UINT32 source;
		UINT32 translated;

		loadingPackage.read((char*) &id, 4);
		loadingPackage.read((char*) &source, 4);
		loadingPackage.read((char*) &translated, 4);

		if (source <= 1)
		{
			continue;
		}

		PackageText* data = new PackageText;
		data->sourceLen = (source - 1);
		data->source = new char[source];

		loadingPackage.read(data->source, source);

		if (translated > 1)
		{
			data->translationLen = (translated - 1);
			data->translation = new char[translated];
			loadingPackage.read(data->translation, translated);
		}
		else
		{
			data->translationLen = 0;
			data->translation = NULL;
		}

		m_textDatabase.insert(std::pair<UINT32, PackageText*>(id, data));

		if (id >= m_nextTextId)
		{
			m_nextTextId = (id + 1);
		}
	}

	// UI
	loadingPackage.seekg(header.uiOffset);

	for (UINT32 i = 0; i < header.uiCount; i++)
	{
		UINT32 id;
		UINT32 source;
		UINT32 translated;

		loadingPackage.read((char*)&id, 4);
		loadingPackage.read((char*)&source, 4);
		loadingPackage.read((char*)&translated, 4);

		if (source <= 1)
		{
			// what
			continue;
		}

		PackageText* data = new PackageText;
		data->sourceLen = (source - 1);
		data->source = new char[source];

		loadingPackage.read(data->source, source);

		if (translated > 1)
		{
			data->translationLen = (translated - 1);
			data->translation = new char[translated];
			loadingPackage.read(data->translation, translated);
		}
		else
		{
			data->translationLen = 0;
			data->translation = NULL;
		}

		m_uiDatabase.insert(std::pair<UINT32, PackageText*>(id, data));

		if (id >= m_nextUIId)
		{
			m_nextUIId = (id + 1);
		}
	}

	// Images
	loadingPackage.seekg(header.imageOffset);

	for (UINT32 i = 0; i < header.imageCount; i++)
	{
		PackageImage* pkg = new PackageImage;
		UINT32 titleLen = 0;

		pkg->hash = new char[41];
		pkg->hash[40] = '\x00';

		loadingPackage.read(pkg->hash, 40);
		loadingPackage.read((char*)&titleLen, 4);

		pkg->title = new char[titleLen];
		pkg->titleLen = titleLen - 1;

		loadingPackage.read(pkg->title, titleLen);

		loadingPackage.read((char*)&pkg->width, 2);
		loadingPackage.read((char*)&pkg->height, 2);
		loadingPackage.read((char*)&pkg->flags, 4);
		loadingPackage.read((char*)&pkg->depth, 4);
		loadingPackage.read((char*)&pkg->replacementLen, 4);

		if (pkg->replacementLen > 0)
		{
			pkg->replacement = new unsigned char[pkg->replacementLen];
			loadingPackage.read((char*)pkg->replacement, pkg->replacementLen);
		}
		else
		{
			pkg->replacement = NULL;
		}

		m_imageDatabase.insert(std::pair<const char*, PackageImage*>(pkg->hash, pkg));
	}

	for (std::map<UINT32, PackageText*>::const_iterator it = m_textDatabase.begin(); it != m_textDatabase.end(); it++)
	{
		TextLookupMap::const_iterator mp = m_fastTextLookup.find(it->second->sourceLen);

		if (mp != m_fastTextLookup.end())
		{
			mp->second->push_back(it->second);
		}
		else
		{
			std::vector<PackageText*>* vec = new std::vector<PackageText*>;

			vec->push_back(it->second);

			m_fastTextLookup.emplace(std::pair<int, std::vector<PackageText*>*>(it->second->sourceLen, vec));
		}
	}

	m_logger->WriteLine("Package file loaded successfully");

	loadingPackage.close();
}

void CResourceManager::SavePackage(const char* filename)
{
	int size = MultiByteToWideChar(CP_UTF8, NULL, filename, -1, NULL, 0);
	wchar_t* widefile = new wchar_t[size];
	MultiByteToWideChar(CP_UTF8, NULL, filename, -1, widefile, size);

	this->SavePackage(widefile);

	delete[] widefile;
}

void CResourceManager::SavePackage(const std::wstring& filename)
{
	m_modified = false;

	std::ofstream writer(filename, std::wios::out | std::wios::binary);
	if (!writer)
	{
		m_logger->WriteLine("Unable to save package file");
		return;
	}

	PackageHeader header;
	header.magicHeader = PACKAGE_MAGIC_HEADER;
	header.charactersCount = 0;
	header.charactersOffset = 0;
	header.reserved1 = 0;
	header.reserved2 = 0;
	header.reserved3 = 0;
	header.reserved4 = 0;

	// Make place for header
	writer.write((char*) &header, sizeof(PackageHeader));

	// Text
	header.textOffset = (UINT32) writer.tellp();
	header.textCount = 0;

	for (std::map<UINT32, PackageText*>::const_iterator it = m_textDatabase.begin(); it != m_textDatabase.end(); it++)
	{
		UINT32 lenSource = it->second->sourceLen;
		UINT32 lenTranslated = it->second->translationLen;
		
		if (lenSource <= 0)
		{
			continue;
		}

		lenSource++;

		writer.write((char*) &it->first, 4); // id
		writer.write((char*) &lenSource, 4);

		if (lenTranslated <= 0)
		{
			lenTranslated = 0;
		}
		else
		{
			lenTranslated++;
		}
		
		writer.write((char*) &lenTranslated, 4);

		writer << it->second->source << '\x00';

		if (lenTranslated > 0)
		{
			writer << it->second->translation << '\x00';
		}

		header.textCount++;
	}

	// UI
	header.uiOffset = (UINT32)writer.tellp();
	header.uiCount = 0;

	for (std::map<UINT32, PackageText*>::const_iterator it = m_uiDatabase.begin(); it != m_uiDatabase.end(); it++)
	{
		UINT32 lenSource = it->second->sourceLen;
		UINT32 lenTranslated = it->second->translationLen;

		if (lenSource <= 0)
		{
			continue;
		}

		lenSource++;

		writer.write((char*)&it->first, 4);
		writer.write((char*)&lenSource, 4);

		if (lenTranslated <= 0)
		{
			lenTranslated = 0;
		}
		else
		{
			lenTranslated++;
		}

		writer.write((char*)&lenTranslated, 4);

		writer << it->second->source << '\x00';

		if (lenTranslated > 0)
		{
			writer << it->second->translation << '\x00';
		}

		header.uiCount++;
	}

	// Images
	header.imageOffset = (UINT32)writer.tellp();
	header.imageCount = 0;

	for (ImageMap::const_iterator it = m_imageDatabase.begin(); it != m_imageDatabase.end(); it++)
	{
		if (strlen(it->second->hash) != 40 || it->second->titleLen <= 0)
		{
			m_logger->WriteLine("Invalid image hash or title");
			continue;
		}

		writer.write(it->second->hash, 40);

		UINT32 titleLen = it->second->titleLen + 1;
		UINT32 replacementLen = it->second->replacementLen;

		writer.write((char*)&titleLen, 4);

		writer << it->second->title << '\x00';

		writer.write((char*)&it->second->width, 2);
		writer.write((char*)&it->second->height, 2);
		writer.write((char*)&it->second->flags, 4);
		writer.write((char*)&it->second->depth, 4);

		writer.write((char*)&it->second->replacementLen, 4);

		if (it->second->replacementLen > 0)
		{
			writer.write((char*)it->second->replacement, it->second->replacementLen);
		}

		header.imageCount++;
	}

	writer.seekp(0);
	writer.write((char*) &header, sizeof(PackageHeader));

	writer.close();

	m_logger->WriteLine("Package file successfully saved");
}