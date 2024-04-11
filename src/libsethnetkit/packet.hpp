/*
 * SPDX-FileCopyrightText: 2023 AllWorldIT
 *
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Base Packet Class
class Packet {
	protected:
		// std::vector<uint8_t> rawData;
		std::vector<uint8_t> payload;

	private:
		void _clear();

	public:
		Packet();
		Packet(const std::vector<uint8_t> &data);

		virtual ~Packet();

		void clear();

		void parse(const std::vector<uint8_t> &data);

		void addPayload(const std::vector<uint8_t> &data);
		const uint8_t *getPayloadPointer() const;
		const std::vector<uint8_t> &getPayload() const;
		uint16_t getPayloadSize() const;
		void resizePayload(uint16_t newSize);

		virtual uint16_t getHeaderOffset() const;
		virtual uint16_t getHeaderSize() const;
		virtual uint16_t getPacketSize() const;

		void printHex() const;
		void printText() const;

		bool compare(std::string bin);

		virtual std::string asHex() const;
		virtual std::string asText() const;
		virtual std::string asBinary() const;
};