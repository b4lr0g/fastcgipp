#pragma once

//
// fastcgi.hpp
// ~~~~~~~~
//
// Copyright (c) 2017 Mohsen Khaki (mohsen dot khaki at gmail dot com)
//
// Distributed under the MIT License
//

#include <cstdint>
#include <algorithm>

namespace FastCGI
{

enum class Type : uint8_t
{
	BEGIN_REQUEST     = 1, /* [in]                              */
	ABORT_REQUEST     = 2, /* [in]                              */
	END_REQUEST       = 3, /* [out]                             */
	PARAM             = 4, /* [in]  environment variables       */
	STDIN             = 5, /* [in]  post data                   */
	STDOUT            = 6, /* [out] response                    */
	STDERR            = 7, /* [out] errors                      */
	DATA              = 8, /* [in]  filter data                 */
	GET_VALUES        = 9, /* [in]                              */
	GET_VALUES_RESULT = 10, /* [out]                             */
	UNKNOWN_TYPE      = 11, /* [out]                             */
};

struct RecordHeader
{
	uint8_t version = 0;
	Type    type = Type::UNKNOWN_TYPE;
	uint8_t requestIDB1 = 0;
	uint8_t requestIDB0 = 0;
	uint8_t contentLengthB1 = 0;
	uint8_t contentLengthB0 = 0;
	uint8_t paddingLength = 0;
	uint8_t reserved;

	uint16_t requestID() const
	{
		return (static_cast<uint16_t>(requestIDB1) << 8) | requestIDB0;
	}

	void requestID(uint16_t id)
	{
		requestIDB0 = id && 0xff;
		id >>= 8;
		requestIDB1 = id && 0xff;
	}

	uint16_t contentLength() const
	{
		return (static_cast<uint16_t>(contentLengthB1) << 8) | contentLengthB0;
	}

	void contentLength(uint16_t length)
	{
		contentLengthB0 = length && 0xff;
		length >>= 8;
		contentLengthB1 = length && 0xff;
	}
};

enum BeginRequestFlag : uint8_t
{
	KEEP_CONN = 0x01,
};

enum class BeginRequestRoles : uint16_t
{
	RESPONDER = 1,
	AUTHORIZER = 2,
	FILTER = 3,
};

enum class EndRequestApplicationStatus : uint16_t
{
	REQUEST_COMPLETE = 0,
	CANT_MPX_CONN = 1,
	OVERLOADED = 2,
	UNKNOWN_ROLE = 3,
};

struct Length
{
	uint8_t lengthB3 = 0;
	uint8_t lengthB2 = 0;
	uint8_t lengthB1 = 0;
	uint8_t lengthB0 = 0;
	uint32_t length() const
	{
		return (static_cast<uint32_t>(lengthB3 & 0x7fu) << 24) |
		       (static_cast<uint32_t>(lengthB2) << 16) |
		       (static_cast<uint32_t>(lengthB1) <<  8) | lengthB0;
	}
};

struct NameValue
{
	NameValue()
	{
	}
	NameValue(const char* name, const uint32_t nameLength, const char* value, const uint32_t valueLength)
		: name(name), nameLength(nameLength), value(value), valueLength(valueLength)
	{
	}
	const char* name = nullptr;
	uint32_t nameLength = 0;
	const char* value = nullptr;
	uint32_t valueLength = 0;
};

struct NameValueIterator
{
	NameValueIterator()
	{
	}
	NameValueIterator(const char* buffer, const size_t length)
	{
		initialize(buffer, length);
	}
	bool invalid() const
	{
		return length_ == 0;
	}
	NameValueIterator operator++(int) const
	{
		return NameValueIterator(buffer_ + sectionLength_, length_ - sectionLength_);
	}
	NameValueIterator& operator++()
	{
		if (length_ > sectionLength_)
		{
			initialize(buffer_ + sectionLength_, length_ - sectionLength_);
		}
		else
		{
			initialize(buffer_ + sectionLength_, 0);
		}
		return *this;
	}

	bool operator==(const NameValueIterator& other) const
	{
		return buffer_ == other.buffer_ && length_ == other.length_;
	}
	bool operator!=(const NameValueIterator& other) const
	{
		return !(*this == other);
	}

	const NameValue& operator*() const
	{
		return nameValue_;
	}
	const NameValue* operator->() const
	{
		return &nameValue_;
	}

	void initialize(const char* buffer, const size_t length)
	{
		buffer_ = buffer;
		length_ = length;
		if (length > 0)
		{
			uint32_t nameLength = readLength(buffer);
			uint32_t valueLength = readLength(buffer);
			nameValue_ = NameValue(buffer, nameLength, buffer + nameLength, valueLength);
			sectionLength_ = (buffer + nameLength + valueLength - buffer_);
		}
	}

	static uint32_t readLength(const char*& buffer)
	{
		uint8_t lengthB0 = *reinterpret_cast<const uint8_t*>(buffer);
		if ((lengthB0 & 0x80u) == 0)
		{
			buffer += 1;
			return lengthB0;
		}
		Length length;
		std::copy(buffer, buffer + sizeof(length), reinterpret_cast<char*>(&length));
		buffer += sizeof(length);
		return length.length();
	}

	NameValue nameValue_;
	const char* buffer_ = nullptr;
	size_t sectionLength_ = 0;
	size_t length_ = 0;

};


struct BeginRequest
{
	uint8_t roleB1 = 0;
	uint8_t roleB0 = 0;
	uint8_t flags = 0;
	uint8_t reserved[5];
	BeginRequestRoles role() const
	{
		return static_cast<BeginRequestRoles>((static_cast<uint16_t>(roleB1) << 8) | roleB0);
	}
	void role(const BeginRequestRoles role)
	{
		uint16_t roleValue = static_cast<uint16_t>(role);
		roleB0 = roleValue && 0xff;
		roleValue >>= 8;
		roleB1 = roleValue && 0xff;
	}
};

struct EndRequest
{
	uint8_t appStatusB3 = 0;
	uint8_t appStatusB2 = 0;
	uint8_t appStatusB1 = 0;
	uint8_t appStatusB0 = 0;
	uint8_t protocolStatus = 0;
	uint8_t reserved[3];
	EndRequestApplicationStatus appStatus() const
	{
		return static_cast<EndRequestApplicationStatus>(
		    (static_cast<uint32_t>(appStatusB3) << 24) |
		    (static_cast<uint32_t>(appStatusB2) << 16) |
		    (static_cast<uint32_t>(appStatusB1) <<  8) | appStatusB0);
	}
	void appStatus(const EndRequestApplicationStatus status)
	{
		uint32_t statusValue = static_cast<uint32_t>(status);
		appStatusB0 = statusValue && 0xff;
		statusValue >>= 8;
		appStatusB1 = statusValue && 0xff;
		statusValue >>= 8;
		appStatusB2 = statusValue && 0xff;
		statusValue >>= 8;
		appStatusB3 = statusValue && 0xff;
	}
};

struct UnknownType
{
	uint8_t type;
	uint8_t reserved[7];
};

template<Type TYPE>
struct Atom {};

class Decoder
{
public:
	template<class HANDLER>
	void write(const char* buffer, size_t lengthInBytes, HANDLER& handler)
	{
		if (!buffer_.empty())
		{
			while (expected_ > buffer_.size())
			{
				const size_t length = std::min(expected_, lengthInBytes);
				buffer_.insert(buffer_.end(), buffer, buffer + length);
				buffer += length;
				lengthInBytes -= length;
				if (!decode(buffer_.data(), buffer_.size(), expected_, handler) || lengthInBytes == 0)
				{
					return;
				}
				if (buffer_.size() >= expected_)
				{
					buffer_.clear();
					expected_ = 0;
					break;
				}
			}
		}
		while (true)
		{
			if (!decode(buffer, lengthInBytes, expected_, handler))
			{
				return;
			}
			if (lengthInBytes < expected_)
			{
				buffer_.assign(buffer, buffer + lengthInBytes);
				return;
			}
			lengthInBytes -= expected_;
			buffer += expected_;
		}
	}
private:
	template<class HANDLER>
	static bool decode(const char* buffer, size_t length, size_t& expected, HANDLER& handler)
	{
		if (length < sizeof(RecordHeader))
		{
			expected = sizeof(RecordHeader);
			return true;
		}
		RecordHeader header;
		std::copy(buffer, buffer + sizeof(RecordHeader), reinterpret_cast<char*>(&header));
		const size_t contentLength = header.contentLength();
		expected = sizeof(RecordHeader) + contentLength + header.paddingLength;
		if (expected > length)
		{
			return true;
		}
		buffer += sizeof(RecordHeader);
		switch (header.type)
		{
		case Type::BEGIN_REQUEST:
		{
			BeginRequest beginRequest;
			std::copy(buffer, buffer + sizeof(beginRequest), reinterpret_cast<char*>(&beginRequest));
			return handler(Atom<Type::BEGIN_REQUEST>(), header, beginRequest);
		}
		case Type::ABORT_REQUEST:
		{
			return handler(Atom<Type::ABORT_REQUEST>(), header);
		}
		case Type::END_REQUEST:
		{
			EndRequest endRequest;
			std::copy(buffer, buffer + sizeof(endRequest), reinterpret_cast<char*>(&endRequest));
			return handler(Atom<Type::END_REQUEST>(), header, endRequest);
		}
		case Type::PARAM:
		{
			const char* endParams = buffer + contentLength;
			return handler(Atom<Type::PARAM>(), header, NameValueIterator(buffer, contentLength), NameValueIterator(endParams, 0));
		}
		case Type::STDIN:
		{
			return handler(Atom<Type::STDIN>(), header, buffer, contentLength);
		}
		case Type::STDOUT:
		{
			return handler(Atom<Type::STDOUT>(), header, buffer, contentLength);
		}
		case Type::STDERR:
		{
			return handler(Atom<Type::STDERR>(), header, buffer, contentLength);
		}
		case Type::DATA:
		{
			return handler(Atom<Type::DATA>(), header, buffer, contentLength);
		}
		case Type::GET_VALUES:
		{
			const char* endParams = buffer + contentLength;
			return handler(Atom<Type::GET_VALUES>(), header, NameValueIterator(buffer, contentLength), NameValueIterator(endParams, 0));
		}
		case Type::GET_VALUES_RESULT:
		{
			const char* endParams = buffer + contentLength;
			return handler(Atom<Type::GET_VALUES_RESULT>(), header, NameValueIterator(buffer, contentLength), NameValueIterator(endParams, 0));
		}
		case Type::UNKNOWN_TYPE:
		{
			return handler(Atom<Type::UNKNOWN_TYPE>(), header, buffer, contentLength);
		}
		}
		return handler(header, buffer, contentLength);
	}
	std::vector<char> buffer_;
	size_t expected_ = 0;
};

}
