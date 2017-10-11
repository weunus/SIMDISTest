#include "InitDLPS.h"
#include <QApplication>
#include "StdCapture.h"
#include "dlps/DLPSString.h"
#include "dlps/DLPSCore.h"
#include "DLPSUnitDomainMap.h"
#include "TDBRecordSchemaMap.h"
#include "LKDataElemDict.h"
#include "LKMessageMap.h"
#include "MHIMessageMap.h"

void InitDLPS::run()
{
	DLPS_CONFIG.Set("!TRACK NUMBER, OWN UNIT", 8);

	DLPSUnitDomainMap *domainMap = (DLPSUnitDomainMap *)DLPS_UNIT_DOMAIN_MAP;
	TDBRecordSchemaMap *recordSchemaMap = (TDBRecordSchemaMap *)TDB_RECORD_SCHEMA_MAP;
	LKDataElemDict* dataElemDict = (LKDataElemDict*)LK_DATA_ELEM_DICT;
	MHIMessageMap *mhiMessageMap = (MHIMessageMap *)MHI_MESSAGE_MAP;

	STD_CAPTURE->BeginCapture();
	emit sigInitialize(0);

	int total = 4;
	int i = 1;

	bool success = false;
	if (domainMap->Initialize())
	{
		emit sigInitialize((i++) * 100 / total);

		if (recordSchemaMap->Initialize())
		{
			emit sigInitialize((i++) * 100 / total);
			if (dataElemDict->Initialize())
			{
				emit sigInitialize((i++) * 100 / total);
				if (mhiMessageMap->Initialize())
				{
					emit sigInitialize((i++) * 100 / total);
					success = true;
				}
			}
		}
	}

	STD_CAPTURE->EndCapture();

	QString errString;
	if (!success)
	{
		errString = STD_CAPTURE->GetCapture();
		emit sigInitialize(0, errString);
	}
}