#define DCS_SERVERIDSZ 10
#define M_PI 3.1415926535897932384626433832795
#define DEG2RAD 0.017453292519943295769236907684886

enum DCSDataType_t
{
	DCSUNKNOWN = 0,		/**< Unknown data type.  The contents of this packet are undefined. */
	DCSPLATFORMHEADER = 1,	/**< Platform header.  This packet contains data pertaining to a SIMDIS platform header structure. */
	DCSPLATFORMDATA = 2,	/**< Platform data.  This packet contains data pertaining to a SIMDIS platform data structure. */
	DCSBEAMHEADER = 3,	/**< Beam header.  This packet contains data pertaining to a SIMDIS beam header structure. */
	DCSBEAMDATA = 4,	/**< Beam data.  This packet contains data pertaining to a SIMDIS beam data structure. */
	DCSGATEHEADER = 5,	/**< Gate header.  This packet contains data pertaining to a SIMDIS gate header structure. */
	DCSGATEDATA = 6,	/**< Gate data.  This packet contains data pertaining to a SIMDIS gate data structure. */
	DCSTIMEHEADER = 7,	/**< Time header.  This packet contains data pertaining to a SIMDIS time header structure. */
	DCSTIMEDATA = 8,	/**< Time data.  This packet contains data pertaining to a SIMDIS time data structure. */
	DCSSCENARIOHEADER = 9,	/**< Scenario header.  This packet contains data pertaining to a SIMDIS scenario header structure. */
	DCSSCENARIODATA = 10,	/**< Scenario data.  This packet contains data pertaining to a SIMDIS scenario data structure. */
	DCSEVENT = 11,		/**< Event.  This packet contains data pertaining to a SIMDIS event structure. */
	DCSGENERICDATA = 12,	/**< Generic data.  This packet contains data pertaining to a SIMDIS generic data structure. */
	DCSSCOPEDATA = 13,	/**< Scope data.  This packet contains data pertaining to a Plot-XY Scope display. */
	DCSCATEGORYDATA = 14	/**< Category data.  This packet contains data pertaining to a SIMDIS category data entry. */
};

enum DCSCoordSystem
{
	DCSCOORD_WGS84 = 0,	/**< Earth Centered, Earth Fixed Geocentric coordinate system: based on WGS-84. */
	DCSCOORD_LLA = 1,	/**< Geodetic coordinate system with position information specified as x=lat,y=lon,z=alt. */
	DCSCOORD_NED = 2,	/**< Topographic coordinate system with position information specified as x=north,y=east,z=down. */
	DCSCOORD_NWU = 3,	/**< Topographic coordinate system with position information specified as x=north,y=west,z=up. */
	DCSCOORD_ENU = 4,	/**< Topographic coordinate system with position information specified as x=east,y=north,z=up. */
	DCSCOORD_XEAST = 5,	/**< Cartesian tangent plane system with position information specified as x=east,y=north,z=up. */
	DCSCOORD_GTP = 6,       /**< Flat Earth: Generic Tangent Plane, User defined X-Y rotation and tangential offset. */
	DCSCOORD_ECI = 7        /**< Earth Centered Inertial Geocentric coordinate system: based on WGS-84. */
};

///Enumeration of Vertical datum type constants
enum DCSVerticalDatum_t
{
	DCSVERTDATUM_WGS84 = 1,  /**< WGS-84 Ellipsoidal Vertical Datum */
	DCSVERTDATUM_MSL,      /**< Mean Sea Level based on EGM-96 Datum */
	DCSVERTDATUM_NAVD88,   /**< North American Vertical Datum of 1988 (not supported) */
	DCSVERTDATUM_NGVD29,   /**< National Geodetic Vertical Datum of 1929 (Sea Level Datum) (not supported) */
	DCSVERTDATUM_NAD83,    /**< North American Datum of 1983 (not supported) */
	DCSVERTDATUM_MHHW,     /**< Mean Higher High Water Tidal Datum (not supported) */
	DCSVERTDATUM_MHW,      /**< Mean High Water Tidal Datum (not supported) */
	DCSVERTDATUM_DTL,      /**< Diurnal Tide Level Diurnal (not supported) */
	DCSVERTDATUM_MTL,      /**< Mean Tide Level Datum (not supported) */
	DCSVERTDATUM_LMSL,     /**< Local Mean Sea Level Datum (not supported) */
	DCSVERTDATUM_MLW,      /**< Mean Low Water Tidal Datum (not supported) */
	DCSVERTDATUM_MLLW,     /**< Mean Lower Low Water Tidal Datum (not supported)*/
	DCSVERTDATUM_USER      /**< User Defined Vertical Datum Offset */
};

///Enumeration of magnetic variance datum type constants
enum DCSMagneticVariance_t
{
	DCSMAGVAR_TRUE = 1,  /**< No variance, True Heading */
	DCSMAGVAR_WMM,     /**< World Magnetic Model (WMM) */
	DCSMAGVAR_USER     /**< User Defined Magnetic Variance Offset */
};

enum DCSRequestType
{
	DCSREQUEST_CONNECT = 0,     /**< Client requests negotiation of a new connection. */
	DCSREQUEST_VERSION = 1,     /**< Client requests DCS API version. */
	DCSREQUEST_DATAMODE = 2,	/**< Client requests the type of UDP socket used for data transmission. */
	DCSREQUEST_TIME = 3,		/**< Client requests time data in the form of DCSTimeHeader. */
	DCSREQUEST_SCENARIO = 4,	/**< Client requests scenario data in the form of DCSScenarioHeader. */
	DCSREQUEST_EVENT = 5,		/**< Server sends new event to all connected clients.  Not to be used by client for making requests. */
	DCSREQUEST_HEADER = 6,		/**< Server sends new header to all connected clients.  Client requests a header for a specific id.  Also used for sending DCSGenericData and DCSCategoryData over TCP */
	DCSREQUEST_HEADERS = 7,		/**< Client requests headers for all known objects.  Header objects are DCSPlatformHeader, DCSBeamHeader, DCSGateHeader. */
	DCSREQUEST_DISCONNECT = 8,	/**< Client requests to disconnect from server or is ordered to disconnect by server. */
	DCSREQUEST_KEEPALIVE = 9	/**< Client/server check for active connection. */
};
enum DCSEventType_t
{
  DCSEVENT_UNKNOWN=0,	/**< Event type is unknown. */
  DCSEVENT_TARGETID=1,	/**< Event to set target id. */
  DCSEVENT_COLOR=2,		/**< Event to set object color. */
  DCSEVENT_STATE=3,		/**< Event to set object state.
						* @see @ref state "Object States".
						*/
  DCSEVENT_STATUS=4,		/**< Event to set track status.
						* @see @ref status "Track Status".
						*/
  DCSEVENT_CLASSIFICATION=5,	/**< Event to set classification level for scenario. */
  DCSEVENT_BEAMINTERPPOS=6,	/**< Event to set beam interpolate position flag */
  DCSEVENT_GATEINTERPPOS=7,	/**< Event to set gate interpolate position flag */
  DCSEVENT_PLATFORMINTERPOLATE=8,	/**< Event to set platform interpolate orientation flag */
  DCSEVENT_CLASSIFICATIONSTR=9,	/**< Event to set classification string for scenario. */
  DCSEVENT_CALLSIGN=10,		/**< Event to set the call sign for platform, beam or gate. */
  DCSEVENT_ICONNAME=11		/**< Event to set the icon name for a platform. */
};

enum DCSUDPMode_t
{
	DCSDATA_UNCONNECTED = 0,	/**< No UDP socket, socket is invalid. */
	DCSDATA_UNICAST = 1,		/**< Standard Unicast socket. */
	DCSDATA_BROADCAST = 2,		/**< Broadcast socket. */
	DCSDATA_MULTICAST = 3		/**< Multicast socket. */
};

enum DCSTrackStatus_t
{
	DCSSTATUS_DROPPED = 0,	/**< Track has been dropped (no good). */
	DCSSTATUS_LIVE = 1	/**< Track is live (good). */
};

enum DCSObjectState_t
{
	DCSSTATE_SAME = 0,	/**< State has not changed. */
	DCSSTATE_ON = 1,	/**< Toggle object state to on. */
	DCSSTATE_OFF = 2,	/**< Toggle object state to off (end time is set). */
	DCSSTATE_EXPIRE = 3	/**< Expired object, remove from both server & client. */
};

enum DCSBeamType_t
{
	DCSBEAM_UNKNOWN = 0,		/**< Unknown beam type. */
	DCSBEAM_STEADY = 1,			/**< Steady beam. */
	DCSBEAM_CONICALSPIRAL = 2,	/**< Conical spiral beam. (not supported) */
	DCSBEAM_HORIZBIDIR = 3,		/**< Horizontal bi-directional beam. (not supported) */
	DCSBEAM_VERTBIDIR = 4,		/**< Vertical bi-directional beam. (not supported) */
	DCSBEAM_HORIZUNIDIR = 5,	/**< Horizontal uni-directional beam. (not supported) */
	DCSBEAM_VERTUNIDIR = 6,		/**< Vertical uni-directional beam. (not supported) */
	DCSBEAM_HORIZRASTER = 7,	/**< Horizontal raster beam. (not supported) */
	DCSBEAM_VERTRASTER = 8,		/**< Vertical raster beam. (not supported) */
	DCSBEAM_LINEAR = 9,			/**< Linear beam. */
	DCSBEAM_BODYAXIS = 10		/**< Platform body axis beam. */
};

#define CNT_MODEL_NAME 20
static char* strAirCraft[CNT_MODEL_NAME] =
{
	"a-10a_thunderbolt",
	"ac-130h_spectre",
	"ah-1f_cobra",
	"ah-64d_longbow",
	"av-8b_harrier",
	"b-52h_stratofortress",
	"bell-412",
	"boeing_757-200",
	"c-5_galaxy",
	"c-12j_huron",
	"c-17a_globemaster",
	"c-17a_globemaster_open_cargo",
	"c-26d",
	"c-130_hercules",
	"ch-53e_sea_stallion",
	"cp-140_aurora",
	"e-2c_hawkeye",
	"e-3a_sentry",
	"e-8c_jstars",
	"ea-6b_prowler",
};

static char* strShip[CNT_MODEL_NAME] =
{
	"boghammar",
	"ddg-1000",
	"hmcs_halifax_class_frigate",
	"hmcs_iroquois_class_destroyer",
	"hmcs_victoria",
	"ins_krivak-III_talwar",
	"jds_kirishima_ddg_174",
	"jds_kongo_ddg_173",
	"kmrss",
	"MATSS",
	"type_021_huangfeng",
	"usns_t-agos_class",
	"uss_arleigh_burke_class_ddg",
	"uss_arleigh_burke_class_ddg_flt_II",
	"uss_arleigh_burke_class_ddg_flt_IIa",
	"uss_arleigh_burke_class_ddg_mod_flt_IIa",
	"uss_austin_class_lpd",
	"uss_ffg_class",
	"uss_henry_kaiser_class_t-ao",
	"uss_lcac_80"
};

static char* strVehicles[CNT_MODEL_NAME] =
{
	"bv-206_giraffe_radar",
	"c-802_launcher",
	"crf250r_motorcycle_insurgents",
	"hmmwv_avenger_camo",
	"hmmwv_m1037_des",
	"lav-25_des",
	"m1a2_abrams",
	"m35a2_truck_des",
	"m109a6_paladin_des",
	"m113a2_gavin_des",
	"mitsubishi_2004_lancer",
	"pac-3_patriot_launcher",
	"sa-6_launcher_des",
	"sa-13_launcher_des",
	"scania_tour_bus",
	"shahab-3_launcher_deployed",
	"t72_tank_des",
	"thaad_launcher",
	"toyota_pickuptruck_1980",
	"zsu-23-4_shilka"
};

static char*  strMissiles[1] =	//TO DO : CNT_MODEL_NAME 20°³
{
	"agm-84_harpoon"
};