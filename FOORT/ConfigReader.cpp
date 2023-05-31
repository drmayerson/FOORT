#include "ConfigReader.h"

#include <ios> // for std::scientific
#include <type_traits> // for std::is_same_v

using namespace	ConfigReader;

//////////////////////////////////////////
///// Accessors to navigate the collection

int ConfigCollection::GetSettingIndex(std::string_view SettingName) const
{
	// Return -1 if setting not found
	int retIndex{ -1 };

	// Iterate through Settings vector until found
	int i{ 0 };
	bool found{ false };
	while (i < m_Settings.size() && !found)
	{
		if (m_Settings[i].SettingName == SettingName)
		{
			// We have found the setting
			retIndex = i;
			found = true;
		}
		++i;
	}

	return retIndex;
}

bool ConfigCollection::Exists(std::string_view SettingName) const
{
	// Setting index returns -1 if setting not found
	return GetSettingIndex(SettingName) >= 0;
}

bool ConfigCollection::IsCollection(int SettingIndex) const
{
	// Makes sure index is within range of number of settings, and then checks to see if this setting is a collection
	return SettingIndex >= 0 && SettingIndex < m_Settings.size()
		&& std::holds_alternative<std::unique_ptr<ConfigCollection>>(m_Settings[SettingIndex].SettingValue);
}

bool ConfigCollection::IsCollection(std::string_view SettingName) const
{
	// Defer to index-based implementation
	return IsCollection( GetSettingIndex(SettingName) );
}

const ConfigCollection& ConfigCollection::operator[](int CollectionIndex) const
{
	// This should only be called with a valid index of a collection, otherwise throw an exception
	if (IsCollection(CollectionIndex))
	{
		// Return reference to the subcollection
		return *std::get<std::unique_ptr<ConfigCollection>>(m_Settings[CollectionIndex].SettingValue);
	}
	else
	{
		throw ConfigReaderException("Invalid collection");
	}
}

const ConfigCollection& ConfigCollection::operator[](std::string_view CollectionName) const
{
	// Defer to index-based implemntation
	return operator[]( GetSettingIndex(CollectionName) );
}

int ConfigCollection::NrSettings() const
{
	// We don't expect so many settings that an int wouldn't suffice!
	return static_cast<int>(m_Settings.size());
}


/////////////////////////////////////
///// Accessors to get setting values

// Templated functions implemented in .h file!

bool ConfigCollection::LookupValueInteger(std::string_view SettingName, int& theOutput) const
{
	// If the output is int, we only look up to see if the value is an int
	return LookupValue(SettingName, theOutput);
}

bool ConfigCollection::LookupValueInteger(std::string_view SettingName, long& theOutput) const
{
	// If the output is long, the setting value can either be a long or an int, so we check for both

	long longtry{};
	if (LookupValue(SettingName, longtry))
	{
		theOutput = longtry;
		return true;
	}

	int inttry{};
	if (LookupValueInteger(SettingName, inttry))
	{
		theOutput = static_cast<long>(inttry);
		return true;
	}

	return false;
}

bool ConfigCollection::LookupValueInteger(std::string_view SettingName, long long& theOutput) const
{
	// If the output is long long, the setting value can either be a long long, a long, or an int, so we check for all

	long long lltry{};
	if (LookupValue(SettingName, lltry))
	{
		theOutput = lltry;
		return true;
	}

	// Note that we don't have to check separately for long and int; these are both implemented already in the long
	// version of LookupValueInteger
	long longtry{};
	if (LookupValueInteger(SettingName, longtry))
	{
		theOutput = static_cast<long long>(longtry);
		return true;
	}

	return false;
}

bool ConfigCollection::LookupValueInteger(std::string_view SettingName, unsigned int& theOutput) const
{
	// An int that is >=0 will always fit in an unsigned int
	int inttry{};
	if (LookupValueInteger(SettingName, inttry) && inttry >= 0)
	{
		theOutput = static_cast<unsigned int>(inttry);
		return true;
	}

	// If the value is an integral type other than an int, we can put it in an unsigned int if it fits
	long long lltry{};
	if (LookupValueInteger(SettingName, lltry) && lltry >= 0 && lltry <= std::numeric_limits<unsigned int>::max())
	{
		theOutput = static_cast<unsigned int>(lltry);
		return true;
	}

	return false;
}

bool ConfigCollection::LookupValueInteger(std::string_view SettingName, unsigned long& theOutput) const
{
	// A long that is >=0 will always fit in an unsigned long
	long longtry{};
	if (LookupValueInteger(SettingName, longtry) && longtry >= 0)
	{
		theOutput = static_cast<unsigned long>(longtry);
		return true;
	}

	// If the value is an integral type other than an long, we can put it in an unsigned long if it fits
	long long lltry{};
	if (LookupValueInteger(SettingName, lltry) && lltry >= 0 && lltry <= std::numeric_limits<unsigned long>::max())
	{
		theOutput = static_cast<unsigned long>(lltry);
		return true;
	}

	return false;
}

bool ConfigCollection::LookupValueInteger(std::string_view SettingName, unsigned long long& theOutput) const
{
	// A long long that is >=0 will always fit in an unsigned long long
	long long lltry{};
	if (LookupValueInteger(SettingName, lltry) && lltry >= 0)
	{
		theOutput = static_cast<unsigned long long>(lltry);
		return true;
	}

	return false;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////
///// Output entire collection (including subcollections) to a given outputstream, and helper functions

void ConfigCollection::DisplayCollection(std::ostream& OutputStream, int Indent) const
{
	// Simply loop through all settings and output them
	// (The indentation is necessary to indent subcollections correctly)
	for (int i = 0; i < m_Settings.size(); ++i)
		DisplaySetting(OutputStream, i, Indent);
}

void ConfigCollection::DisplayTabs(std::ostream& OutputStream, int NrTabs) const
{
	// Helper function to output given number of tabs
	for (int i = 0; i < NrTabs; ++i)
		OutputStream << '\t';
}

void ConfigCollection::DisplaySetting(std::ostream& OutputStream, int SettingIndex, int Indent) const
{
	// Helper function to output single, given setting

	// First off-set to the correct indentation
	DisplayTabs(OutputStream, Indent);
	
	// Display "SettingName = "
	OutputStream << m_Settings[SettingIndex].SettingName << " = ";
	
	// Now, depending on the type of the setting, the output to the right of the "=" will be different
	if (IsCollection(SettingIndex))
	{
		// The setting is a subcollection

		OutputStream << '\n'; // end the line

		// Display { at the correct indentation
		DisplayTabs(OutputStream, Indent);
		OutputStream << "{\n";

		// Call DisplayCollection() on the subcollection (this will loop through its settings), at one indentation higher
		std::get<std::unique_ptr<ConfigCollection>>(m_Settings[SettingIndex].SettingValue)->DisplayCollection(OutputStream,
			Indent + 1);
		
		// Display } at the correct indentation
		DisplayTabs(OutputStream, Indent);
		OutputStream << "}";
	}
	else if (std::holds_alternative<bool>(m_Settings[SettingIndex].SettingValue))
	{
		// The setting is a boolean: display "true" of "false"

		if (std::get<bool>(m_Settings[SettingIndex].SettingValue))
			OutputStream << "true";
		else
			OutputStream << "false";
	}
	else if (std::holds_alternative<std::string>(m_Settings[SettingIndex].SettingValue))
	{
		// The setting is a string: display it, surrounded by ""

		OutputStream << '"' << std::get<std::string>(m_Settings[SettingIndex].SettingValue) << '"';
	}
	else if (std::holds_alternative<double>(m_Settings[SettingIndex].SettingValue))
	{
		// The setting is a double: display it in scientific notation
		OutputStream << std::scientific << std::get<double>(m_Settings[SettingIndex].SettingValue);
	}
	else // integral type
	{
		// The setting is of an integral type (int, long, long long)
		// Simply display it to screen
		// Use std::visit with single lambda that can be used for all types
		std::visit(
			[&OutputStream](auto&& arg)
			{
				using intType = std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<intType, int> || std::is_same_v<intType, long> || std::is_same_v<intType, long long>)
					OutputStream << arg;
			},
			m_Settings[SettingIndex].SettingValue);
	}

	// Display trailing ; and end the line
	OutputStream << ";\n";
}


////////////////////////////////////////////////
///// Read in a config file and helper functions

// Main function: read in config file of given file name
// Returns true if successful, false if file does not exist
// Will throw exception if parse/syntax error encountered
bool ConfigCollection::ReadFile(const std::string& FileName)
{
	// Open the file
	std::ifstream inFile{ FileName };

	// If unsuccessful opening file, return false
	if (!inFile)
		return false;

	// Set up try block to catch any exceptions (syntax errors etc) raised by reading in this root collection
	try
	{
		// File was successfully opened, read in the entire (root) collection from the file stream
		ReadCollection(inFile);
	}
	catch (ConfigReaderException const& ex)
	{
		// Something went wrong in reading in the root collection;
		// throw exception back up the stack, but
		// use the information in the trace vector to alter the message according to where it came from
		std::string errmsg{ "ConfigReader error: " };
		errmsg += ex.what();
		std::vector<int> thetrace = ex.trace();
		errmsg += " Occurred when reading in root setting " + std::to_string(thetrace.back() + 1);
		thetrace.pop_back();
		while (thetrace.size() > 0)
		{
			errmsg += ", sub-setting " + std::to_string(thetrace.back() + 1);
			thetrace.pop_back();
		}
		throw ConfigReaderException(errmsg, ex.trace());
	}

	// Collection succesfully read in
	return true;
}


// Read in entire collection (including subcollections)
// Pre: stream is positioned right after { (or at beginning of file)
// Post: stream is positioned right after } (or at EOF)	
void ConfigCollection::ReadCollection(std::ifstream & InputFile)
{
	// First, empty the settings vector
	m_Settings = std::vector<ConfigSetting>{};

	// set up try block to catch any ConfigReaderException that are thrown in reading in collection
	try
	{
		// Keep looping and reading in new settings as long as we encounter them
		bool EndCollection{ false };
		while (!EndCollection)
		{
			// Read in a new setting
			ConfigSetting NewSetting{};

			// First read in the setting name
			ReadSettingName(InputFile, NewSetting.SettingName);

			// If there is no setting left in this collection, then
			// ReadSettingName() returns "" (and has eaten up the '}' if applicable)
			if (NewSetting.SettingName == "")
				EndCollection = true;
			else
			{
				// There is another setting, and its name has been read in
				// Now read in the '='
				ReadSettingSpecificChar(InputFile, '=');
				// Then, read in the setting value (the rhs of the '=')
				ReadSettingValue(InputFile, NewSetting.SettingValue);
				// Finally, at the end of a setting, we always expect ';'
				ReadSettingSpecificChar(InputFile, ';');

				// Make sure this setting does not exist yet!
				if (Exists(NewSetting.SettingName))
					throw ConfigReaderException("Duplicate setting \"" + NewSetting.SettingName + "\" detected.");

				// Setting has been completely read in, move it to the settings vector
				m_Settings.push_back(std::move(NewSetting));
			}
		}
	}
	catch (ConfigReaderException const& ex)
	{
		// Something has gone wrong while reading in this collection!
		// We will throw the exception back up the stack, but add in information about the place at which it happened
		std::vector<int> newtrace{ ex.trace() };
		newtrace.push_back(static_cast<int>(m_Settings.size()));
		throw ConfigReaderException(ex.what(), newtrace );
	}

	// Post condition is true because ReadSettingName() has eaten up } in last iteration of loop (for subcollections)
}


// Read in a setting name
// Pre: next non-whitespace character in stream is beginning of name
// Post: stream has just read in all characters of name (but no more);
// returns "" if there is no more setting to read in the current collection, in this case
// '}' has been read in (for subcollections)
void ConfigCollection::ReadSettingName(std::ifstream & InputFile, std::string& theName)
{
	// Eat up any leading white space
	InputFile >> std::ws;
	
	// Empty name to start
	theName = "";
	
	// Keep looping and reading in characters as long as there are valid characters in the name
	int NextChar{}; // peek() returns int and not char because it can also be EOF
	bool WordFinished{ false };
	while (!WordFinished)
	{
		// Peek at next character in file
		NextChar = InputFile.peek();

		// We need to check for various cases of this next character

		// If theName is still empty, then we might encounter the end of collection ('}' or EOF)
		// If the next character is EOF or '}', then
		// it signifies the end of the current collection
		if (theName == "" && (NextChar == EOF || NextChar == '}'))
		{
			// Remove this next character from the stream
			InputFile.get();
			WordFinished = true;
		}
		// If theName is still empty, then we might encounter a comment ("//...")
		// If the next two characters are "//", then we throw away the rest of the current line since it's a comment
		// Note: If we only have one '/' but no second '/', this is a syntax error
		else if (theName == "" && NextChar == '/')
		{
			// Throw away first '/'
			InputFile.get();
			// Is the next character also '/'?
			if (InputFile.peek() == '/')
			{
				// Throw away rest of line after "//"
				InputFile.ignore(MaxStreamSize, '\n');
				// Eat up any leading whitespace of following line
				InputFile >> std::ws;
			}
			else
			{
				// Lonely '/' encountered, syntax error!
				throw ConfigReaderException("Bad character in setting name.");
			}
		}
		// The character could also be a valid character of the name
		// i.e. either a letter (alpha) or a digit (if it is NOT the first character in the name) or an underscore
		else if (std::isalpha(NextChar) || (std::isdigit(NextChar) && theName != "") || NextChar == '_')
		{
			// Remove this character from the input stream and add it to the name string
			InputFile.get();
			theName += static_cast<char>(NextChar);
		}
		else
		{
			// We have encountered something other then another valid name character, OR something other than a valid name
			// character or end of collection if the name is still ""
			// In any case: this is the end of the name!
			WordFinished = true;
			// If the name is empty, it cannot be a valid name, so throw exception!
			// Note that the end of collection ('}' or EOF) was checked for already above
			if (theName == "")
				throw ConfigReaderException("Invalid setting name.");

			// Note: we have NOT removed this (invalid name) character from the stream!
		}
	}

	// Post condition is met: every (valid) name character has been read in, but the character immediately after the name
	// has NOT been read in, EXCEPT if it is '}' or EOF signifying the end of the collection.
}


// Read in one specific character only (not '/')
// Pre: next non-whitespace character in stream is this character
// Post: stream is just after character
void ConfigCollection::ReadSettingSpecificChar(std::ifstream & InputFile, char theChar) const
{
	// Eat up any leading white space
	InputFile >> std::ws;
	
	// Read the next character
	// get() returns int and not char because it can also be EOF
	int NextChar{ InputFile.get() };

	// We must discount any possible comments "//...."
	while (NextChar == '/') // First character read in is '/'
	{
		// Get the next character
		NextChar = InputFile.get();
		// If this is also '/', then we have a comment
		if (NextChar == '/')
		{
			// Comment: ignore the rest of the line
			InputFile.ignore(MaxStreamSize, '\n');

			// Eat up leading white space in new line
			InputFile >> std::ws;
			// Read in first non-white space character of this new line
			NextChar = InputFile.get();
		}
		else
		{
			// only one '/' encountered: syntax error!
			throw ConfigReaderException(std::string("Expected ") + static_cast<char>(theChar) + " not found.");
		}
	}
	
	// Check to see if the character is indeed the one we are looking for;
	// if it isn't, syntax error!
	if (NextChar != theChar)
	{
		throw ConfigReaderException(std::string("Expected ") + static_cast<char>(theChar) + " not found.");
	}

	// Post condition is met: specific character has been eaten up from stream
}


// Read in setting value (could be subcollection)
// Pre: next non-whitespace character in stream is beginning of value
// (can be '{' for subcollection, digit or '-' or '.' or 'e' for number, '"' for string, or 't'/'f' for boolean)
// Post: Value has entirely been read in (i.e. next character should be ';')
void ConfigCollection::ReadSettingValue(std::ifstream & InputFile, ConfigSettingValue &theValue)
{	
	// Eat up any leading white space
	InputFile >> std::ws;
	
	// peek() returns int and not char because it can also be EOF
	int NextChar = InputFile.peek();
	
	// First, we must check for comments
	while (NextChar == '/')
	{
		// First character is '/', remove this from stream
		InputFile.get();
		// Get next character from stream
		NextChar = InputFile.get();
		// This must also be '/' for comment
		if (NextChar == '/')
		{
			// We have a comment, ignore the rest of the line
			InputFile.ignore(MaxStreamSize, '\n');
			
			// Eat up leading white space in next line and peek at first non-ws character
			InputFile >> std::ws;
			NextChar = InputFile.peek();
		}
		else
		{
			// Only one '/': this cannot be start of valid setting; syntax error!
			throw ConfigReaderException("Invalid setting value");
		}
	}
	
	// We can now start reading in the setting
	// The first character determines what type of setting it is
	if (NextChar == '"') 
	{
		// Value type is string; create string to hold value in
		std::string theValueString{""};
		
		// Eat up '"' in stream
		InputFile.get(); 
		
		// Now, read in string until closing '"' is encountered
		NextChar = InputFile.get();
		while (NextChar != '"')
		{
			// Check for EOF
			if (!InputFile)
				throw ConfigReaderException("Unexpected EOF when reading setting value");
			
			if (NextChar == '\\') // escape next character
			{
				// Discard current '\' and add NEXT character to string, no matter what it is
				theValueString += static_cast<char>(InputFile.get());
			}
			else
			{
				// Add current character to string
				theValueString += static_cast<char>(NextChar);
			}
			
			// Read in next character
			NextChar = InputFile.get();
		}
		
		// String has been completely read in (including closing '"'), put it in theValue
		theValue = std::move(theValueString);
	}
	else if ( std::isdigit(NextChar) || NextChar == '.' || NextChar == '-' || NextChar == 'e' )
	{
		// Value type is some kind of number
		
		// First, we will read in the entire value as a string
		std::string theValueString{ "" };
		
		// Read in number, character by character; we use peek as we do NOT want to move the stream
		// beyond the last character in the number
		while (InputFile && (std::isdigit(NextChar) || NextChar == '.' || NextChar == '-' || NextChar == 'e') )
		{
			// Add character to number string
			theValueString += static_cast<char>(NextChar);
			
			// Throw current character away and peek at next one
			InputFile.get();
			NextChar = InputFile.peek();
		}
		// if EOF was encountered, this is an error
		if (!InputFile)
			throw ConfigReaderException("Unexpected EOF when reading setting value");

		// We have now read in the number as a string.
		// Is it of integral type (int, long, long long) or a double?
		// To determine this, we see if the number string contains a decimal or 'e', which signifies a double
		if (theValueString.find('.') == std::string::npos && theValueString.find('e') == std::string::npos)
		{
			// We have NOT found a decimal or 'e' so the number is of integral type
			
			// We will now convert the number string to the SMALLEST possible integral type (of int, long, long long)
			// in which it fits: i.e. first we try int, then long, then long long.
			// If the conversion fails at a given step due to out of range, then we go on to the next-smallest type
			try
			{
				// Try converting to int first
				int theValueInt = std::stoi(theValueString);
				theValue = std::move(theValueInt);
			}
			catch(std::out_of_range const &)
			{
				try
				{
					// Converting to int failed due to number too large, so try converting to long
					long theValueLong = std::stol(theValueString);
					theValue = std::move(theValueLong);
				}
				catch(std::out_of_range const &)
				{
					try
					{
						// Converting to long failed due to number too large, so convert to long long
						long long theValuell = std::stoll(theValueString);
						theValue = std::move(theValuell);
					}
					catch (std::out_of_range const&)
					{
						// Too big even for long long!
						throw ConfigReaderException("Setting value (integral number) too large to fit in long long.");
					}
				}
			}
			catch (std::invalid_argument const&)
			{
				// stoi failed because this is actually not a valid number
				throw ConfigReaderException("Invalid setting value.");
			}
		}
		else
		{
			// We HAVE found a decimal or 'e' so the number must be a double.
			// Convert to double and put it in theValue
			try
			{
				double theValueDouble = std::stod(theValueString);
				theValue = std::move(theValueDouble);
			}
			catch (std::invalid_argument const&)
			{
				// stod failed because the number string is not valid
				throw ConfigReaderException("Invalid setting value.");
			}
		}
	}
	else if (NextChar == '{')
	{
		// Value type is collection
		// Throw out leading '{'
		InputFile.get();
		
		// Create new collection and read it in from current location, then put it in theValue
		std::unique_ptr<ConfigCollection> theValueCollection = std::make_unique<ConfigCollection>();
		theValueCollection->ReadCollection(InputFile);
		theValue = std::move(theValueCollection);

		// Note that ReadCollection, through ReadSettingName, has eaten up the closing '}'
	}
	else if (NextChar == 't' || NextChar == 'f')
	{
		// Value type is boolean

		if (NextChar == 't') // should be "true"
		{
			// Read in all letters of "true", starting with 't'
			InputFile.get(); // 't'
			NextChar = InputFile.get();
			if (NextChar != 'r')
				throw ConfigReaderException("Invalid setting value");
			NextChar = InputFile.get();
			if (NextChar != 'u')
				throw ConfigReaderException("Invalid setting value");
			NextChar = InputFile.get();
			if (NextChar != 'e')
				throw ConfigReaderException("Invalid setting value");

			theValue = true;
		}
		else
		{
			// Read in all letters of "false", starting with 'f'
			InputFile.get(); // 'f'
			NextChar = InputFile.get();
			if (NextChar != 'a')
				throw ConfigReaderException("Invalid setting value");
			NextChar = InputFile.get();
			if (NextChar != 'l')
				throw ConfigReaderException("Invalid setting value");
			NextChar = InputFile.get();
			if (NextChar != 's')
				throw ConfigReaderException("Invalid setting value");
			NextChar = InputFile.get();
			if (NextChar != 'e')
				throw ConfigReaderException("Invalid setting value");

			theValue = false;
		}
	}
	else
	{
		// Must be invalid setting value, since we do not recognize it as any of the allowed types!
		throw ConfigReaderException("Invalid setting value");
	}
	
	// Post condition met since value has entirely been read in, but no other characters have been read in
	// (i.e. next character should be ';')
}
	
	
	


