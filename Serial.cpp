#include "Serial.hpp"

bool ExitSimuSerial = false;

void Serial::setPort(std::string p) {
	portString = p;
}

void Serial::read() {

	// opening serial port
	asio::io_service io;
	asio::serial_port port(io);
	//OutputDebugStringA(portString.c_str());

	try {
		port.open(portString);
		port.set_option(asio::serial_port_base::baud_rate(38400));
		// getting response string (until \n character)
		rsp = "";
		do {
			asio::read(port, asio::buffer(&c, 1));
			rsp += c;
		} while (c != '\n');

		port.close();

		outputFile << rsp; // log to output file

		rsp.erase(std::remove(rsp.begin(), rsp.end(), '\n'), rsp.end());

		// extracting coordinates from response
		pos = 0;
		isEffectorPos = false;
		isBallPos = false;
		isSpinnerPos = false;
		isOpponentPos = false;
		coordCount = 0;
		while ((pos = rsp.find(delimiter)) != std::string::npos) {
			item = rsp.substr(0, pos);
			std::cout << item << '\n';
			recordCoordinate(item);

			if (item == "e") {
				isEffectorPos = true;
				isBallPos = false;
				isSpinnerPos = false;
				isOpponentPos = false;
				coordCount = 0;
			}
			if (item == "b") {
				isEffectorPos = false;
				isBallPos = true;
				isSpinnerPos = false;
				isOpponentPos = false;
				coordCount = 0;
			}
			if (item == "s") {
				isEffectorPos = false;
				isBallPos = false;
				isSpinnerPos = true;
				isOpponentPos = false;
				coordCount = 0;
			}
			if (item == "o") {
				isEffectorPos = false;
				isBallPos = false;
				isSpinnerPos = false;
				isOpponentPos = true;
				coordCount = 0;
			}

			rsp.erase(0, pos + delimiter.length());
		}
		recordCoordinate(rsp);
	}
	catch (boost::system::system_error& e)
	{
		ExitSimuSerial = true;
	}
}

void Serial::write(char ch) {

	// opening serial port
	asio::io_service io;
	asio::serial_port port(io);
	//OutputDebugStringA(portString.c_str());
	try {
		port.open(portString);
		port.set_option(asio::serial_port_base::baud_rate(38400));

		unsigned char command[1] = { 0 };
		command[0] = ch;
		asio::write(port, asio::buffer(command, 1));
	}
	catch (boost::system::system_error& e)
	{
		ExitSimuSerial = true;
	}
}

void Serial::recordCoordinate(std::string item) {
	if (isEffectorPos && coordCount == 0) effectorX = std::stof(item);
	if (isEffectorPos && coordCount == 1) effectorY = std::stof(item);
	if (isBallPos && coordCount == 0) ballX = std::stof(item);
	if (isBallPos && coordCount == 1) ballY = std::stof(item);
	if (isSpinnerPos && coordCount == 0) spinnerAlpha = std::stof(item); 
	if (isOpponentPos && coordCount == 0) opponentX = std::stof(item);
	if (isOpponentPos && coordCount == 1) opponentY = std::stof(item);

	coordCount++;
}

void Serial::openOutputFile() {
	std::time_t t = std::time(0);
	outputFile.open("output_" + std::to_string(t) + ".csv", std::ios_base::app);
}

Serial::~Serial() {
	outputFile.close();
}