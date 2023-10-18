#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

class SequenceDataGenerator {
	private:
		std::string data;

	public:
		SequenceDataGenerator(int len);

		void generate(int len);

		std::string asString() const;
		std::vector<uint8_t> asBytes() const;
};