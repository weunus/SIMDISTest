#include <math.h>
#include <time.h>
#include <sys/timeb.h>
#include <qvector3d.h>

#define HTGIS_WGS84_EARTH_MAJOR_AXIS 6378137.0
#define HTGIS_WGS84_EARTH_MINOR_AXIS 6356752.31424
#define HTGIS_WGS84_EARTH_FLATNESS 0.0033528106647474807198455286185206 // (1.0 / HTGIS_WGS84_EARTH_FLATTENING)
#define HTGIS_DECIMAL_DEGREES_TO_RADIAN 0.017453292519943295769236907684883 // (HTGIS_PI / 180.0)
#define HTGIS_PI 3.1415926535897932384626433832795
#define HTGIS_DOUBLE_PRECISION_EPSILON 1e-12
#define HTGIS_TWO_PI 6.283185307179586476925286766559 // (HTGIS_PI * 2.0)
#define HTGIS_RADIAN_TO_DECIMAL_DEGREES 57.295779513082320876798154814114 // (180.0 / HTGIS_PI)

double GetDistance(double sourceLat, double sourceLong, double destLat, double destLong)
{
	double a = HTGIS_WGS84_EARTH_MAJOR_AXIS, b = HTGIS_WGS84_EARTH_MINOR_AXIS, f = HTGIS_WGS84_EARTH_FLATNESS; // WGS-84 ellipsoid
	double a2b2b2 = (a * a - b * b) / (b * b);

	double omega = (destLong - sourceLong) * HTGIS_DECIMAL_DEGREES_TO_RADIAN;

	double tan_u1 = (1.0 - f) * tan(sourceLat * HTGIS_DECIMAL_DEGREES_TO_RADIAN);
	double cos_u1 = 1.0 / sqrt(1.0 + tan_u1 * tan_u1), sin_u1 = tan_u1 * cos_u1;

	double tan_u2 = (1.0 - f) * tan(destLat * HTGIS_DECIMAL_DEGREES_TO_RADIAN);
	double cos_u2 = 1.0 / sqrt(1.0 + tan_u2 * tan_u2), sin_u2 = tan_u2 * cos_u2;

	double sin_u1_sin_u2 = sin_u1 * sin_u2;
	double cos_u1_sin_u2 = cos_u1 * sin_u2;
	double sin_u1_cos_u2 = sin_u1 * cos_u2;
	double cos_u1_cos_u2 = cos_u1 * cos_u2;

	double lambda = omega;

	double A = 0, sigma = 0, delta_sigma = 0;
	double B, C;
	double lambda0;

	double sin_lambda, cos_lambda;
	double sin_alpha, alpha, cos_alpha, cos_sq_alpha;
	double sin_sq_sigma, sin_sigma, cos_sigma;
	double cos_2_sigma_m, cos_sq_2_sigma_m;
	double u_sq;

	double alpha1 = 0;
	//double alpha2; // [PAULCHO] This is the reverse azimuth. It's simply something we don't need.
	if (sourceLat > destLat)
	{
		alpha1 = HTGIS_PI;
		//alpha2 = 0;
	}
	else if (sourceLat < destLat)
	{
		alpha1 = 0;
		//alpha2 = HTGIS_PI;
	}
	else
	{
		// [PAULCHO] We should never get here!
	}

	bool converged = false;
	for (int i = 0; !converged && i < 10; i++)
	{
		lambda0 = lambda;

		sin_lambda = sin(lambda);
		cos_lambda = cos(lambda);

		sin_sq_sigma = (cos_u2 * sin_lambda * cos_u2 * sin_lambda)
			+ (cos_u1_sin_u2 - sin_u1_cos_u2 * cos_lambda)
			* (cos_u1_sin_u2 - sin_u1_cos_u2 * cos_lambda);

		sin_sigma = sqrt(sin_sq_sigma);

		cos_sigma = sin_u1_sin_u2 + (cos_u1_cos_u2 * cos_lambda);

		sigma = atan2(sin_sigma, cos_sigma);

		if (fabs(sin_sq_sigma) <= HTGIS_DOUBLE_PRECISION_EPSILON)
			sin_alpha = 0;
		else
			sin_alpha = cos_u1_cos_u2 * sin_lambda / sin_sigma;

		alpha = asin(sin_alpha);
		cos_alpha = cos(alpha);
		cos_sq_alpha = cos_alpha * cos_alpha;

		if (fabs(cos_sq_alpha) <= HTGIS_DOUBLE_PRECISION_EPSILON)
			cos_2_sigma_m = 0;
		else
			cos_2_sigma_m = cos_sigma - 2.0 * sin_u1_sin_u2 / cos_sq_alpha;
		u_sq = cos_sq_alpha * a2b2b2;

		cos_sq_2_sigma_m = cos_2_sigma_m * cos_2_sigma_m;

		A = 1.0 + u_sq / 16384.0 * (4096.0 + u_sq * (-768.0 + u_sq * (320.0 - 175.0 * u_sq)));
		B = u_sq / 1024.0 * (256.0 + u_sq * (-128.0 + u_sq * (74.0 - 47.0 * u_sq)));

		delta_sigma = B * sin_sigma * (cos_2_sigma_m + B / 4.0 * (cos_sigma * (-1.0 + 2.0 * cos_sq_2_sigma_m) - B / 6.0 * cos_2_sigma_m * (-3.0 + 4.0 * sin_sq_sigma) * (-3.0 + 4.0 * cos_sq_2_sigma_m)));

		C = f / 16.0 * cos_sq_alpha * (4.0 + f * (4.0 - 3.0 * cos_sq_alpha));

		lambda = omega + (1.0 - C) * f * sin_alpha * (sigma + C * sin_sigma * (cos_2_sigma_m + C * cos_sigma * (-1.0 + 2.0 * cos_sq_2_sigma_m)));

		if (i > 1 && fabs((lambda - lambda0) / lambda) < HTGIS_DOUBLE_PRECISION_EPSILON)
		{
			alpha1 = atan2(cos_u2 * sin_lambda, cos_u1_sin_u2 - sin_u1_cos_u2 * cos_lambda);
			//alpha2 = atan2(cos_u1 * sin_lambda, -sin_u1_cos_u2 + cos_u1_sin_u2 * cos_lambda) + HTGIS_PI;
			converged = true;
		}
	}

	while (alpha1 < 0)
		alpha1 += HTGIS_TWO_PI;
	while (alpha1 >= HTGIS_TWO_PI)
		alpha1 -= HTGIS_TWO_PI;

	//bearing = alpha1 * HTGIS_RADIAN_TO_DECIMAL_DEGREES;
	return (b * A * (sigma - delta_sigma));
}
double GetBearing(double sourceLat, double sourceLong, double destLat, double destLong)
{
	double a = HTGIS_WGS84_EARTH_MAJOR_AXIS, b = HTGIS_WGS84_EARTH_MINOR_AXIS, f = HTGIS_WGS84_EARTH_FLATNESS; // WGS-84 ellipsoid
	double a2b2b2 = (a * a - b * b) / (b * b);

	double omega = (destLong - sourceLong) * HTGIS_DECIMAL_DEGREES_TO_RADIAN;

	double tan_u1 = (1.0 - f) * tan(sourceLat * HTGIS_DECIMAL_DEGREES_TO_RADIAN);
	double cos_u1 = 1.0 / sqrt(1.0 + tan_u1 * tan_u1), sin_u1 = tan_u1 * cos_u1;

	double tan_u2 = (1.0 - f) * tan(destLat * HTGIS_DECIMAL_DEGREES_TO_RADIAN);
	double cos_u2 = 1.0 / sqrt(1.0 + tan_u2 * tan_u2), sin_u2 = tan_u2 * cos_u2;

	double sin_u1_sin_u2 = sin_u1 * sin_u2;
	double cos_u1_sin_u2 = cos_u1 * sin_u2;
	double sin_u1_cos_u2 = sin_u1 * cos_u2;
	double cos_u1_cos_u2 = cos_u1 * cos_u2;

	double lambda = omega;

	double A = 0, sigma = 0, delta_sigma = 0;
	double B, C;
	double lambda0;

	double sin_lambda, cos_lambda;
	double sin_alpha, alpha, cos_alpha, cos_sq_alpha;
	double sin_sq_sigma, sin_sigma, cos_sigma;
	double cos_2_sigma_m, cos_sq_2_sigma_m;
	double u_sq;

	double alpha1 = 0;
	//double alpha2; // [PAULCHO] This is the reverse azimuth. It's simply something we don't need.
	if (sourceLat > destLat)
	{
		alpha1 = HTGIS_PI;
		//alpha2 = 0;
	}
	else if (sourceLat < destLat)
	{
		alpha1 = 0;
		//alpha2 = HTGIS_PI;
	}
	else
	{
		// [PAULCHO] We should never get here!
	}

	bool converged = false;
	for (int i = 0; !converged && i < 10; i++)
	{
		lambda0 = lambda;

		sin_lambda = sin(lambda);
		cos_lambda = cos(lambda);

		sin_sq_sigma = (cos_u2 * sin_lambda * cos_u2 * sin_lambda)
			+ (cos_u1_sin_u2 - sin_u1_cos_u2 * cos_lambda)
			* (cos_u1_sin_u2 - sin_u1_cos_u2 * cos_lambda);

		sin_sigma = sqrt(sin_sq_sigma);

		cos_sigma = sin_u1_sin_u2 + (cos_u1_cos_u2 * cos_lambda);

		sigma = atan2(sin_sigma, cos_sigma);

		if (fabs(sin_sq_sigma) <= HTGIS_DOUBLE_PRECISION_EPSILON)
			sin_alpha = 0;
		else
			sin_alpha = cos_u1_cos_u2 * sin_lambda / sin_sigma;

		alpha = asin(sin_alpha);
		cos_alpha = cos(alpha);
		cos_sq_alpha = cos_alpha * cos_alpha;

		if (fabs(cos_sq_alpha) <= HTGIS_DOUBLE_PRECISION_EPSILON)
			cos_2_sigma_m = 0;
		else
			cos_2_sigma_m = cos_sigma - 2.0 * sin_u1_sin_u2 / cos_sq_alpha;
		u_sq = cos_sq_alpha * a2b2b2;

		cos_sq_2_sigma_m = cos_2_sigma_m * cos_2_sigma_m;

		A = 1.0 + u_sq / 16384.0 * (4096.0 + u_sq * (-768.0 + u_sq * (320.0 - 175.0 * u_sq)));
		B = u_sq / 1024.0 * (256.0 + u_sq * (-128.0 + u_sq * (74.0 - 47.0 * u_sq)));

		delta_sigma = B * sin_sigma * (cos_2_sigma_m + B / 4.0 * (cos_sigma * (-1.0 + 2.0 * cos_sq_2_sigma_m) - B / 6.0 * cos_2_sigma_m * (-3.0 + 4.0 * sin_sq_sigma) * (-3.0 + 4.0 * cos_sq_2_sigma_m)));

		C = f / 16.0 * cos_sq_alpha * (4.0 + f * (4.0 - 3.0 * cos_sq_alpha));

		lambda = omega + (1.0 - C) * f * sin_alpha * (sigma + C * sin_sigma * (cos_2_sigma_m + C * cos_sigma * (-1.0 + 2.0 * cos_sq_2_sigma_m)));

		if (i > 1 && fabs((lambda - lambda0) / lambda) < HTGIS_DOUBLE_PRECISION_EPSILON)
		{
			alpha1 = atan2(cos_u2 * sin_lambda, cos_u1_sin_u2 - sin_u1_cos_u2 * cos_lambda);
			//alpha2 = atan2(cos_u1 * sin_lambda, -sin_u1_cos_u2 + cos_u1_sin_u2 * cos_lambda) + HTGIS_PI;
			converged = true;
		}
	}

	while (alpha1 < 0)
		alpha1 += HTGIS_TWO_PI;
	while (alpha1 >= HTGIS_TWO_PI)
		alpha1 -= HTGIS_TWO_PI;

	return alpha1 * HTGIS_RADIAN_TO_DECIMAL_DEGREES;
	//return (b * A * (sigma - delta_sigma));
}
#define HTGIS_DECIMAL_DEGREES_TO_RADIAN 0.017453292519943295769236907684883 // (HTGIS_PI / 180.0)
#define HTGIS_METERS_PER_FOOT 0.3047999995367040007042099189296 // (1.0 / HTGIS_FEET_PER_METER)
double ConvertToRadians(double decimal)
{
	return decimal * HTGIS_DECIMAL_DEGREES_TO_RADIAN;
}
double ConvertFeetToMeters(double feet)
{
	return feet * HTGIS_METERS_PER_FOOT;
}
DLPSString GetValue(const TDBRecord record, const char* strFieldName/*, bool bUnit*/)
{
	TDBField field;
	if (!record.FindField(strFieldName, field))
	{
		return "";
	}

	DLPSString fieldName = strFieldName;
	unsigned long long code = field.GetCode();

	if (fieldName == "EMITTER NUMBER")
	{
		DLPSString strValue;
		strValue.Format("%05o", code);
		return strValue;
	}
	else if ((fieldName == "MODE II CODE") ||
		(fieldName == "MODE III CODE") ||
		(fieldName == "MODE III CODE, 1") ||
		(fieldName == "4-DIGIT MODE I CODE"))
	{
		DLPSString strValue;
		strValue.Format("%04o", code);
		return strValue;
	}
	else if (fieldName == "ISDL TRACK NUMBER")
	{
		DLPSString strValue;
		strValue.Format("%06o", code);
		return strValue;
	}
	//else if (fieldName == "MDIL TRACK NUMBER")
	//{
	//	DLPSString strTN;
	//	if (GetMDILTNString(false, code, strTN))
	//	{
	//		return strTN;
	//	}
	//}
	else if (fieldName == "LINK-16 TRACK NUMBER")
	{
		JReferenceTN tn(code);
		if (tn.IsNoStatement())
		{
			return "NO STATEMENT";
		}
		else if (tn.IsIllegal())
		{
			return "ILLEGAL";
		}
		else
		{
			return tn.GetText();
		}
	}
	else if ((fieldName == "UNIT REFERENCE NUMBER") || (fieldName == "ENTITY ID SERIAL NUMBER"))
	{
		return DLPSString().Format("%d", code).GetData();
	}
	else
	{
		JDataElemKey key;
		QString strName = strFieldName;
		if (strName == "LATITUDE")
		{
			key.SetKey(281, 14);
		}
		else if (strName == "LONGITUDE")
		{
			key.SetKey(282, 14);
		}
		else if (strName == "ALTITUDE")
		{
			key.SetKey(365, 33);
		}
		else if (strName == "ELEVATION")
		{
			key.SetKey(1612, 1);
		}
		else if (strName == "COURSE")
		{
			key.SetKey(371, 15);
		}
		else if (strName == "DEPTH")
		{
			key.SetKey(366, 8);
		}
		else if (strName == "X POSITION IN WGS-84")
		{
			key.SetKey(1108, 1);
		}
		else if (strName == "Y POSITION IN WGS-84")
		{
			key.SetKey(1108, 2);
		}
		else if (strName == "Z POSITION IN WGS-84")
		{
			key.SetKey(1108, 3);
		}
		else
		{
			return "";
		}

		const JDataElem* dataElem = LK_DATA_ELEM_DICT->FindDataElem(key);
		if (dataElem)
		{
			const JDataItemSet* diSet = dataElem->GetDataItemSet();

			if (diSet)
			{
				const JDataItem* di = diSet->FindDataItem(code);
				if (di)
				{
					DLPSNumber number;

					JDIType diType = di->GetType();
					if (diType == JDITypeEnumeration || diType == JDITypeRange)
					{
						DLPSString text;
						if (di->GetEnumerationText(text))
						{
							return text.GetData();
						}
					}
					else if (diType == JDITypeCharacters)
					{
						DLPSString text;
						if (di->GetCharacters(code, text))
						{
							return text.GetData();
						}
					}
					else if (diType == JDITypeCharacter)
					{
						DLPSString text;
						if (di->GetCharacter(code, text))
						{
							return text.GetData();
						}
					}
					else if (diType == JDITypeNumeric)
					{
						DLPSString unit;
						if (di->GetNumber(code, number, unit))
						{
							DLPSString text;
							if (number.GetType() == DLPSNumber::Integer)
							{
								text.Format("%d", number.GetInteger());
							}
							else
							{
								text.Format("%f", number.GetReal());
							}
							//if ((bUnit) && (!unit.empty()))
							//{
							//	text.Format("%s %s", text.GetData(), unit.GetData());
							//}
							return text.GetData();
						}
					}
					else
					{
						return GetJDITypeName(diType).GetData();
					}
				}
			}
		}
	}

	return "";
}

double Arctangent(double x)
{
	return atan(x) * HTGIS_RADIAN_TO_DECIMAL_DEGREES;
}

void ConvertWGS84XYZToLatitudeLongitudeHeight(double x, double y, double z, double& latitude, double& longitude, double& height)
{
	double a = HTGIS_WGS84_EARTH_MAJOR_AXIS;
	double a2 = a*a;
	double b = HTGIS_WGS84_EARTH_MINOR_AXIS;
	double b2 = b*b;
	double e2 = (a2 - b2) / a2;
	double p = sqrt(x*x + y*y);
	double v = 0.0F;
	double sineLatitude = 0.0F;

	longitude = Arctangent(y / x);
	(longitude<0.0F) ? (longitude += 180.0F) : longitude;
	latitude = Arctangent(z / (p*(1 - e2)));

	sineLatitude = sin(latitude);
	v = a / sqrt(1 - e2*sineLatitude*sineLatitude);

	height = p / cos(latitude) - v;
}

struct timeval {
	long    tv_sec;         /* seconds */
	long    tv_usec;        /* and microseconds */
};
struct timezone {
	int tz_minuteswest;
	int tz_dsttime;
};

int GetTimeOfDay(struct timeval *t, struct timezone *z)
{
	struct _timeb _gtodtmp;
	_ftime(&_gtodtmp);
	(t)->tv_sec = (long)_gtodtmp.time;
	(t)->tv_usec = _gtodtmp.millitm * 1000;

	if (z)
	{
		(z)->tz_minuteswest = _gtodtmp.timezone;
		(z)->tz_dsttime = _gtodtmp.dstflag;
	}

	return 0;
}

double yeartime()
{
	struct timeval tp;

	// get the current system time, using timezone value of 0
	// returns UTC time
	GetTimeOfDay(&tp, 0);

	// put system time into a tm struct
	time_t t(tp.tv_sec);
	struct tm* p_time = gmtime(&t);

	// assemble a UTC "system time"
	unsigned int p_secs = (unsigned int)(p_time->tm_sec) +
		(((unsigned int)(p_time->tm_min)) * 60) +
		(((unsigned int)(p_time->tm_hour)) * 60 * 60) +
		(((unsigned int)(p_time->tm_yday)) * 60 * 60 * 24);

	return (p_secs + (tp.tv_usec * 1e-06));
}

//double dTime = 0.f; //흐르는 시간
//void DrawBallissticMissile(QVector3D StartPos, QVector3D EndPos)
//{
//	double vx, vy, vz;
//	double g; //z축으로의 중력가속도
//	double endTime; //도착지점 도달 시간
//	double maxHeight = 100000; //최대 높이
//	double height; //altitude? // 최대높이의 Z - 시작높이의 Z
//	double endHeight; //도착지점의 높이 Z - 시작지점의 높이 Z
//	double maxTime = 5.0f; //최대 높 이까지 가는 시간.
//
//	endHeight = EndPos.z() - StartPos.z();
//	height = maxHeight - StartPos.z();
//
//	g = 2 * height / (maxTime * maxTime);
//	vz = sqrtf(2 * g * height);
//
//	double a = g;
//	double b = (-2) * vz;
//	double c = 2 * endHeight;
//
//	endTime = (-b + sqrtf(b*b - 4 * a*c)) / (2 * a);
//
//	vx = -(StartPos.x() - EndPos.x()) / endTime;
//	vy = -(StartPos.y() - EndPos.y()) / endTime;
//}