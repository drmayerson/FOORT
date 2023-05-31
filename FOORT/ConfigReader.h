#ifndef _CONFIGREADER_H
#define _CONFIGREADER_H

#include <string_view> // std::string_view
#include <string> // std::string
#include <iostream> // needed for file and console output
#include <fstream> // needed for file input
#include <vector> // needed to create vectors of Settings
#include <variant> // needed for std::variant
#include <stdexcept> // needed for exceptions
#include <limits> // for std::numeric_limits


// Namespace to put all ConfigReader objects
namespace ConfigReader
{
	// Shorthand for the maximum number of characters we can ignore in a stream
	const long long MaxStreamSize{ std::numeric_limits<std::streamsize>::max() };

	// Exception that is thrown if anything bad happens when reading in configuration file
	class ConfigReaderException : public std::runtime_error
	{
	public:
		ConfigReaderException(const std::string& error, std::vector<int> settingtrace = {})
			: std::runtime_error{error.c_str()}, m_settingtrace{settingtrace}
		{}
		// no need to override what() since we can just use std::runtime_error::what()

		std::vector<int> trace() const
		{
			return m_settingtrace;
		};

	private:
		std::vector<int> m_settingtrace;
	};


	// A collection of configuration settings
	class ConfigCollection
	{
	public:		
		///// Accessors to navigate the collection
		// Returns true if the collection contains a setting with the given name
		bool Exists(std::string_view SettingName) const;
		// Returns total number of settings in this collection
		int NrSettings() const;

		// Returns true if the setting with the given name (resp. index) is a collection of subsettings
		// (Returns false if this setting name does not exist, or index is out of bounds)
		bool IsCollection(std::string_view SettingName) const;
		bool IsCollection(int SettingIndex) const;
		// If CollectionName (resp. CollectionIndex) is the name (resp. index)
		// of a setting that is a subcollection, then this returns
		// a const reference to this subcollection; otherwise throws ConfigReaderException
		const ConfigCollection& operator[](std::string_view CollectionName) const;
		const ConfigCollection& operator[](int CollectionIndex) const;


		///// Accessors to get setting values
		// Generic functions that look up the value of a given setting
		// and put it in the output variable
		// Returns true if successful, false if unsuccesful
		// (either due to type mismatch of output and setting value, or if setting doesn't exist)
		// Templated functions implemented below in .h file!
		template<class OutputType>
		bool LookupValue(std::string_view SettingName, OutputType& theOutput) const;
		template<class OutputType>
		bool LookupValue(int SettingIndex, OutputType& theOutput) const;

		// Specific functions for looking up signed integral types
		// These will first see if the setting value is of the given type, then
		// they will try smaller types and then recast to the larger type
		// (so e.g. for long long, first long long is tried, then long, then int)
		bool LookupValueInteger(std::string_view SettingName, int& theOutput) const;
		bool LookupValueInteger(std::string_view SettingName, long& theOutput) const;
		bool LookupValueInteger(std::string_view SettingName, long long& theOutput) const;
		// Specific functions for looking up unsigned integral types
		// These will first test if the setting value is of the signed case, if it is and it is >=0 then it always fits in
		// the unsigned version; then it will test larger (signed) types and see if the result still fits in the unsigned version
		bool LookupValueInteger(std::string_view SettingName, unsigned int& theOutput) const;
		bool LookupValueInteger(std::string_view SettingName, unsigned long& theOutput) const;
		bool LookupValueInteger(std::string_view SettingName, unsigned long long& theOutput) const;
		// Templated functions implemented below in .h file!
		template<class OutputType>
		bool LookupValueInteger(int SettingIndex, OutputType& theOutput) const;


		///// Output entire collection (including subcollections) to a given output stream
		void DisplayCollection(std::ostream& OutputStream, int Indent=0) const;


		///// Read in a config file
		// this overwrites the current collection with the contents of the config file
		// Will throw ConfigReaderException if parse/syntax error in configuration file
		// Returns true if successful
		bool ReadFile(const std::string& FileName);


	private:
		// A configuration setting value can be an integral type, bool, double, string, or a (sub)collection of settings
		// Note: when reading in a configuration file, an integral value will always be stored in the smallest possible type!
		using ConfigSettingValue = std::variant<bool,
			int, long, long long,
			double,
			std::string,
			std::unique_ptr<ConfigCollection> >;
		// A setting is a name and setting value
		struct ConfigSetting
		{
			std::string SettingName;
			ConfigSettingValue SettingValue;
		};
		// The vector of all settings in this collection
		std::vector<ConfigSetting> m_Settings{};


		// Helper function that returns the index of the setting with the given name
		// returns -1 if setting not found
		int GetSettingIndex(std::string_view SettingName) const;


		// Helper functions for DisplayCollection
		// DisplaySetting displays one setting; if it's a subcollection then it calls
		// DisplayCollection on the subcollection
		void DisplaySetting(std::ostream& OutputStream, int SettingIndex, int Indent) const;
		// Output given number of tabs
		void DisplayTabs(std::ostream& OutputStream, int NrTabs) const;


		// Helper functions for ReadFile()
		// These all take an ifstream at a given location and will read in one specific object
		// 
		// Read in entire collection (including subcollections)
		// Pre: stream is positioned right after { (or at beginning of file)
		// Post: stream is positioned right after } (or at EOF)
		void ReadCollection(std::ifstream& InputFile);
		// Read in a setting name
		// Pre: next non-whitespace character in stream is beginning of name
		// Post: stream has just read in all characters of name (but no more);
		// returns "" if there is no more setting to read in the current collection, in this case
		// '}' has been read in (for subcollections)
		void ReadSettingName(std::ifstream& InputFile, std::string& theName);
		// Read in one specific character only (not '/'!)
		// Pre: next non-whitespace character in stream is this character
		// Post: stream is just after character
		void ReadSettingSpecificChar(std::ifstream& InputFile, char theChar) const;
		// Read in setting value (could be subcollection)
		// Pre: next non-whitespace character in stream is beginning of value
		// (can be '{' for subcollection, digit for number, '"' for string, or 't'/'f' for boolean)
		// Post: Value has entirely been read in (i.e. next character should be ';')
		void ReadSettingValue(std::ifstream& InputFile, ConfigSettingValue& theValue);
	};

} // end namespace declarations


///////////////////////////////////////////
///// Implementation of templated functions

template<class OutputType>
bool ConfigReader::ConfigCollection::LookupValue(int SettingIndex, OutputType& theOutput) const
{
	// Get the setting value, if it is indeed of this type
	const OutputType* OutputPointer{ std::get_if<OutputType>(&m_Settings[SettingIndex].SettingValue) };
	if (OutputPointer)
	{
		// Success, the setting is indeed of this type; set the output accordingly
		theOutput = *OutputPointer;
		return true;
	}
	return false;
}

template<class OutputType>
bool ConfigReader::ConfigCollection::LookupValue(std::string_view SettingName, OutputType& theOutput) const
{
	// Look up setting index and then defer to index-based implementation, if setting exists
	int SettingIndex{ GetSettingIndex(SettingName) };
	if (SettingIndex >= 0)
	{
		return LookupValue(SettingIndex, theOutput);
	}
	return false;
}

// This simply defers to the setting-name-based implementation of the function with given output type
template<class OutputType>
bool ConfigReader::ConfigCollection::LookupValueInteger(int SettingIndex, OutputType& theOutput) const
{
	return LookupValueInteger(m_Settings[SettingIndex].SettingName, theOutput);
}

#endif