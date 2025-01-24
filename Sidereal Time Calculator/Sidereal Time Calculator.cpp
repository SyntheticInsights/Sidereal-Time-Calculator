#include <iostream>
#include <iomanip>
#include <cmath>
#include <chrono>
#include <ctime>
#include <sstream>
#include <thread>

using namespace std;
using namespace chrono;

static string convertToReadableTime(double hours) {
	int h = static_cast<int>(hours);
	double fractionalHours = hours - h;

	int m = static_cast<int>(fractionalHours * 60);
	double fractionalMinutes = fractionalHours * 60 - m;

	int s = static_cast<int>(fractionalMinutes * 60);
	double fractionalSeconds = fractionalMinutes * 60 - s;

	stringstream ss;
	ss << setfill('0') << setw(2) << h << ":"
		<< setfill('0') << setw(2) << m << ":"
		<< setfill('0') << setw(2) << s << "."
		<< setfill('0') << setw(8) << fixed << fractionalSeconds;

	return ss.str();
}

static double convertLongitudeToHours(double longitude) {
	return longitude / 15.0;
}

// Function to calculate Julian Date
static double calculateJulianDate(int year, int month, int day, struct tm utcTime, double ms) {
	double a = floor(year / 100.0);
	double b = year >= 1582 ? 2 - a + floor(a / 4.0) : 0;  // Adjust for Gregorian calendar
	double julianDate = (int)(365.25 * (year + 4716)) + (int)(30.6001 * (month + 1)) + day + b - 1524.5
		+ (utcTime.tm_hour / 24.0) + (utcTime.tm_min / 1440.0)
		+ (utcTime.tm_sec / 86400.0) + ms / 86400.0;

	// Correction Factor
	double jcorrection = 2.00000006;
	julianDate += jcorrection;

	return julianDate;
}

static double calculateGST(double julianDate, double hour, double minute, double second, double ms) {
	// GST constants
	double j2st = 6.697374558;  // GST at J2000.0
	double avgChange = 2400.051336;  // Average change in mean sidereal time per Julian Century
	double gmCorrection = 0.000025862;  // Small correction factor for minor changes in Earth's rotation
	double gmConvert = 1.002737909;  // Converts UT into sidereal time

	// GST variables
	double t = (julianDate - 2451545.0) / 36525.0;  // Calculates the Julian Century
	double gmst = j2st + (avgChange * t + gmCorrection * t * t) + (ms / 3600000.0);  // Mean sidereal time
	gmst = fmod(gmst, 24.0);

	// Actual GST
	double ut = hour + minute / 60.0 + second / 3600.0 + ms / 3600000.0;
	double gst = gmst + gmConvert * ut;
	gst = fmod(gst, 24.0);
	if (gst < 0) gst += 24;

	//Correction Factor
	double correctionInHours = (2.0 / 60.0) + (36.2607 / 3600.0) + (1.10034 / 3600.0);
	gst -= correctionInHours;
	if (gst < 0) gst += 24;

	return gst;
}

static double calculateLST(double gst, double longitude) {
	double longHours = convertLongitudeToHours(longitude);
	double lst = gst + longHours;
	if (lst < 0) lst += 24;
	if (lst >= 24) lst -= 24;
	return lst;
}
int main()
{
	// Gets current time
	auto timeNow = system_clock::now();
	auto currentTime = system_clock::to_time_t(timeNow);
	tm utcTime;
	gmtime_s(&utcTime, &currentTime);

	// Precise time including fractional seconds
	auto duration = timeNow.time_since_epoch();
	auto ms = duration_cast<milliseconds>(duration).count() % 1000;

	// Saved Variables from UTC
	int year = utcTime.tm_year + 1900;
	int month = utcTime.tm_mon + 1;
	int day = utcTime.tm_mday;
	int hour = utcTime.tm_hour;
	int minute = utcTime.tm_min;
	int second = utcTime.tm_sec;

	// Display the UTC time including fractional seconds
	cout << "Date: " << setfill('0') << setw(4) << year << '-'
		<< setw(2) << month << '-'
		<< setw(2) << day << ", Time: "
		<< setw(2) << hour << ':'
		<< setw(2) << minute << ':'
		<< setw(2) << second << '.'
		<< setw(3) << ms << endl;

	//J2000.0

	struct tm j2000_tm = { 0 };

	//J2000.0 date and time
	j2000_tm.tm_year = 2000 - 1900;             //Year
	j2000_tm.tm_mon = 0;                        //Month 0-11
	j2000_tm.tm_mday = 1;                       //Day of the Month
	j2000_tm.tm_hour = 12;                      //Hour of the Day
	j2000_tm.tm_min = 0;                        //Minute of the Hour
	j2000_tm.tm_sec = 0;                        //Second of the Minute
	//Convert struct tm to to a time_t
	time_t j2000_time = mktime(&j2000_tm);
	//Thread Safe
	struct tm timeinfo;
	localtime_s(&timeinfo, &j2000_time);
	// J2000.0 Verification
	char jbuffer[26];
	asctime_s(jbuffer, sizeof(jbuffer), &timeinfo);
	cout << "J2000.0 Date & Time: " << jbuffer << endl;

	//Julian Date 

	double julianDate = calculateJulianDate(year, month, day, utcTime, static_cast<double>(ms));
	//Julian Date Verification
	cout << "Julian Date: " << fixed << setprecision(2) << julianDate << endl;

	//Greenwich Sidereal Time (GST)

	while (true) {
		double gst = calculateGST(julianDate, hour, minute, second, static_cast<double>(ms));
		//GST Verification
		cout << "Greenwich Sidereal Time: " << convertToReadableTime(gst) << endl;

		//Local Sidereal Time
		double longitude;
		cout << "Enter longitude: " << endl;
		cin >> longitude;
		while (true) {
			double lst = calculateLST(gst, longitude);
			cout << "\rLocal Sidreal Time: " << convertToReadableTime(lst) << flush;
			gst += 1.0 / 3600.0;
			if (gst >= 24) gst -= 24;
			this_thread::sleep_for(chrono::seconds(1s));
		}
		return 0;

	}
}