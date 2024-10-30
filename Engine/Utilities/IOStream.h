#pragma once
#include "CommonHeaders.h"

namespace lightning::util {
	class BlobStreamReader {
		public:
			DISABLE_COPY_AND_MOVE(BlobStreamReader);

			explicit BlobStreamReader(const u8* buffer) : _buffer{ buffer }, _position{ buffer } {
				assert(buffer);
			}

			template<typename T> [[nodiscard]] constexpr T read() {
				static_assert(std::is_arithmetic_v<T>, "Template argument should be primitive type.");
				T value{ *((T*)_position) };
				_position += sizeof(T);
				return value;
			}

			void read(u8* buffer, size_t length) {
				memcpy(buffer, _position, length);
				_position += length;
			}

			constexpr void skip(size_t offset) { _position += offset; }
			[[nodiscard]] constexpr const u8* const buffer_start() const { return _buffer; }
			[[nodiscard]] constexpr const u8* const position() const { return _position; }
			[[nodiscard]] constexpr size_t const offset() const { return _position - _buffer; }

		private:
			const u8* const _buffer;
			const u8* _position;
	};

	class BlobStreamWriter {
		DISABLE_COPY_AND_MOVE(BlobStreamWriter);

		public:
			explicit BlobStreamWriter(u8* buffer, size_t buffer_size) : _buffer{ buffer }, _position{ buffer }, _buffer_size{ buffer_size } {
				assert(buffer && buffer_size);
			}

			template<typename T> constexpr void write(T value) {
				static_assert(std::is_arithmetic_v<T>, "Template argument should be primitive type.");
				assert(&_position[sizeof(T)] <= &_buffer[_buffer_size]);
				*((T*)_position) = value;
				_position += sizeof(T);
			}

			void write(const char* buffer, size_t length) {
				assert(&_position[length] <= &_buffer[_buffer_size]);
				memcpy(_position, buffer, length);
				_position += length;
			}

			void write(const u8* buffer, size_t length) {
				assert(&_position[length] <= &_buffer[_buffer_size]);
				memcpy(_position, buffer, length);
				_position += length;
			}

			constexpr void skip(size_t offset) {
				assert(&_position[offset] <= &_buffer[_buffer_size]);
				_position += offset;
			}

			[[nodiscard]] constexpr const u8* const buffer_start() const { return _buffer; }
			[[nodiscard]] constexpr const u8* const buffer_end() const { return &_buffer[_buffer_size]; }
			[[nodiscard]] constexpr const u8* const position() const { return _position; }
			[[nodiscard]] constexpr size_t const offset() const { return _position - _buffer; }

		private:
			u8* const _buffer;
			u8* _position;
			size_t _buffer_size;
	};
}