#include "util.hxx"

#include <sstream>
#include <cctype>

using namespace svgdom;

void svgdom::skip_whitespaces(std::istream& s){
	while(!s.eof()){
		if(!std::isspace(s.peek())){
			break;
		}
		s.get();
	}
}

void svgdom::skip_whitespaces_and_comma(std::istream& s){
	bool commaSkipped = false;
	while(!s.eof()){
		if(std::isspace(s.peek())){
			s.get();
		}else if(s.peek() == ','){
			if(commaSkipped){
				break;
			}
			s.get();
			commaSkipped = true;
		}else{
			break;
		}
	}
}

void svgdom::skip_till_char_inclusive(std::istream& s, char c){
	while(!s.eof()){
		if(s.get() == c){
			break;
		}
	}
}

std::string svgdom::read_till_char(std::istream& s, char c){
	std::stringstream ss;
	while(!s.eof()){
		if(s.peek() == c || s.peek() == std::char_traits<char>::eof()){
			break;
		}
		ss << char(s.get());
	}
	return ss.str();
}

std::string svgdom::read_till_char_or_whitespace(std::istream& s, char c){
	std::stringstream ss;
	while(!s.eof()){
		if(std::isspace(s.peek()) || s.peek() == c || s.peek() == std::char_traits<char>::eof()){
			break;
		}
		ss << char(s.get());
	}
	return ss.str();
}


namespace{
std::string readInNumberString(std::istream& s){
	std::stringstream ss;
	
	bool sign = false;
	bool expSign = false;
	bool exponent = false;
	bool dot = false;
	bool validNumber = false;
	
	while(!s.eof()){
		auto c = char(s.peek());
//		TRACE(<< "c = " << char(c) << std::endl)
		switch(c){
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				sign = true;
				if(exponent){
					expSign = true;
				}
				validNumber = true;
				break;
			case '+':
			case '-':
				if(sign){
					if(expSign){
						return ss.str();
					}
					expSign = true;
					if(!exponent){
						return ss.str();
					}
				}
				sign = true;
				break;
			case 'e':
			case 'E':
				sign = true;
				if(exponent){
					return ss.str();
				}
				exponent = true;
				break;
			case '.':
				if(dot || exponent){
					return ss.str();
				}
				sign = true;
				dot = true;
				break;
			default:
				if(!validNumber){
					//WORKAROUND: if no valid number was read then we need to leave stream in failed state
					//to do that, try to read in the float number (we know it should fail since no valid number detected on stream).
					float x;
					s >> x;
				}
				return ss.str();
		}
		ss << char(s.get());
	}
	return ss.str();
}
}


real svgdom::read_in_real(std::istream& s) {
	skip_whitespaces(s);
	
	//On MacOS reading in the number which is terminated by non-number and non-whitespace character,
	//e.g. "0c" will result in stream error, i.e. s.fail() will return true and stream will be in unreadable state.
	//To workaround this, need to read in the number to a separate string and parse it from there.
	auto str = readInNumberString(s);
	
	if(str.length() == 0){
		return 0;
	}
	
	std::istringstream iss(str);
	
	//Visual Studio standard library fails to parse numbers to 'float' if it does not actually fit into 'float',
	//for example, parsing of "5.47382e-48" to float fails instead of giving 0.
	//To workaround this read in to the biggest precision type 'long double'.
	long double x;
	iss >> x;
	return real(x);
}

std::string svgdom::trim_tail(const std::string& s){
	const auto t = s.find_last_not_of(" \t\n\r");
	if(t == std::string::npos){
		return s;
	}
	
	return s.substr(0, t + 1);
}

std::string svgdom::iri_to_local_id(const std::string& iri){
	if(iri.length() != 0 && iri[0] == '#'){
		return iri.substr(1, iri.length() - 1);
	}
	return std::string();
}


coordinate_units svgdom::parse_coordinate_units(const std::string& s){
	if(s == "userSpaceOnUse"){
		return coordinate_units::user_space_on_use;
	}else if(s == "objectBoundingBox"){
		return coordinate_units::object_bounding_box;
	}
	return coordinate_units::unknown;
}


std::string svgdom::coordinate_units_to_string(coordinate_units u){
	switch(u){
		default:
			return std::string();
		case coordinate_units::object_bounding_box:
			return "objectBoundingBox";
		case coordinate_units::user_space_on_use:
			return "userSpaceOnUse";
	}
}


r4::vector2<real> svgdom::parse_number_and_optional_number(const std::string& s, r4::vector2<real> defaults){
	r4::vector2<real> ret;
	
	std::istringstream ss(s);
	skip_whitespaces(ss);
	ret[0] = read_in_real(ss);
	if(ss.fail()){
		return defaults;
	}
	skip_whitespaces_and_comma(ss);

	if(ss.eof()){
		ret[1] = defaults[1];
		return ret;
	}
	
	ret[1] = read_in_real(ss);
	if(ss.fail()){
		ret[1] = defaults[1];
	}
	return ret;
}

std::string svgdom::number_and_optional_number_to_string(std::array<real, 2> non, real optionalNumberDefault){
	std::stringstream ss;
	
	ss << non[0];
	
	if(non[1] != optionalNumberDefault){
		ss << " " << non[1];
	}
	
	return ss.str();
}
