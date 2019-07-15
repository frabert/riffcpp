#include "membuffer.hpp"
#include <fstream>
#include <riffcpp.hpp>

using iter = riffcpp::Chunk::iterator;

class riffcpp::Chunk::iterator::impl {
public:
  std::shared_ptr<std::istream> m_stream = nullptr;
  std::streampos m_pos;
  std::streampos m_limit;
};

class riffcpp::Chunk::impl {
public:
  std::shared_ptr<std::istream> m_stream = nullptr;
  std::shared_ptr<std::streambuf> m_buf = nullptr;
  std::streampos m_pos;
  std::streampos m_limit;

  FourCC m_id;
  std::uint32_t m_size;
};

static riffcpp::FourCC read_id(std::shared_ptr<std::istream> stream,
                               std::streampos chunk_pos) {

  stream->seekg(chunk_pos);
  riffcpp::FourCC read_id;
  stream->read(read_id.data(), read_id.size());
  return read_id;
}

static std::uint32_t read_size(std::shared_ptr<std::istream> stream,
                               std::streampos chunk_pos) {
  std::streamoff offs{4};
  stream->seekg(chunk_pos + offs);
  uint32_t read_size;
  stream->read(reinterpret_cast<char *>(&read_size), 4);
  return read_size;
}

static riffcpp::FourCC read_size_as_id(std::shared_ptr<std::istream> stream,
                                       std::streampos chunk_pos) {
  stream->seekg(chunk_pos + std::streamoff{4});
  riffcpp::FourCC read_id;
  stream->read(read_id.data(), read_id.size());
  return read_id;
}

riffcpp::Chunk::Chunk(const char *filename) {
  pimpl = std::make_unique<riffcpp::Chunk::impl>();
  auto stream = std::make_shared<std::ifstream>(filename, std::ios::binary);

  if (!stream->is_open()) {
    throw Error("Couldn't open specified file", ErrorType::CannotOpenFile);
  }

  pimpl->m_stream = stream;
  pimpl->m_pos = pimpl->m_stream->tellg();

  pimpl->m_stream->seekg(0, std::ios::end);
  pimpl->m_limit = pimpl->m_stream->tellg();

  pimpl->m_id = read_id(stream, pimpl->m_pos);
  pimpl->m_size = read_size(stream, pimpl->m_pos);

  if (pimpl->m_pos + std::streamoff{pimpl->m_size} > pimpl->m_limit) {
    throw Error("Chunk size outside of range", ErrorType::InvalidFile);
  }
}

riffcpp::Chunk::Chunk(const Chunk &ch) {
  pimpl = std::make_unique<riffcpp::Chunk::impl>();
  pimpl->m_buf = ch.pimpl->m_buf;
  pimpl->m_stream = ch.pimpl->m_stream;
  pimpl->m_pos = ch.pimpl->m_pos;
  pimpl->m_limit = ch.pimpl->m_limit;
  pimpl->m_id = ch.pimpl->m_id;
  pimpl->m_size = ch.pimpl->m_size;
}

riffcpp::Chunk &riffcpp::Chunk::operator=(const riffcpp::Chunk &lhs) {
  pimpl->m_buf = lhs.pimpl->m_buf;
  pimpl->m_stream = lhs.pimpl->m_stream;
  pimpl->m_pos = lhs.pimpl->m_pos;
  pimpl->m_limit = lhs.pimpl->m_limit;
  pimpl->m_id = lhs.pimpl->m_id;
  pimpl->m_size = lhs.pimpl->m_size;
  return *this;
}

riffcpp::Chunk::Chunk(const void *buffer, std::size_t len) {
  if (buffer == nullptr) {
    throw Error("Null buffer", ErrorType::NullBuffer);
  }

  pimpl = std::make_unique<riffcpp::Chunk::impl>();

  const char *charbuf = static_cast<const char *>(buffer);

  pimpl->m_buf = std::make_shared<membuffer>(const_cast<char *>(charbuf), len);
  pimpl->m_stream = std::make_shared<std::istream>(pimpl->m_buf.get());
  pimpl->m_pos = pimpl->m_stream->tellg();
  pimpl->m_stream->seekg(0, std::ios::end);
  pimpl->m_limit = pimpl->m_stream->tellg();

  pimpl->m_id = read_id(pimpl->m_stream, pimpl->m_pos);
  pimpl->m_size = read_size(pimpl->m_stream, pimpl->m_pos);

  if (pimpl->m_pos + std::streamoff{pimpl->m_size} > pimpl->m_limit) {
    throw Error("Chunk size outside of range", ErrorType::InvalidFile);
  }
}

riffcpp::Chunk::Chunk(std::unique_ptr<riffcpp::Chunk::impl> &&impl) {
  pimpl = std::move(impl);
}

riffcpp::Chunk::~Chunk() = default;

riffcpp::FourCC riffcpp::Chunk::id() { return pimpl->m_id; }

std::uint32_t riffcpp::Chunk::size() { return pimpl->m_size; }

riffcpp::FourCC riffcpp::Chunk::type() {
  std::streamoff offs{8};
  pimpl->m_stream->seekg(pimpl->m_pos + offs);
  riffcpp::FourCC read_type;
  pimpl->m_stream->read(read_type.data(), read_type.size());
  return read_type;
}

std::vector<char> riffcpp::Chunk::data() {
  std::streamoff offs{8};
  pimpl->m_stream->seekg(pimpl->m_pos + offs);

  std::uint32_t data_size = size();

  std::vector<char> read_data;
  read_data.resize(data_size);

  pimpl->m_stream->read(read_data.data(), data_size);
  return read_data;
}

iter riffcpp::Chunk::begin(bool no_chunk_id) {
  std::streamoff offs{no_chunk_id ? 8 : 12};
  auto it = std::make_unique<riffcpp::Chunk::iterator::impl>();
  it->m_pos = pimpl->m_pos + offs;
  it->m_stream = pimpl->m_stream;
  it->m_limit = pimpl->m_limit;

  return iter{std::move(it)};
}

iter riffcpp::Chunk::end() {
  std::uint32_t sz = size();
  std::streamoff offs{sz + sz % 2 + 8};

  auto it = std::make_unique<riffcpp::Chunk::iterator::impl>();
  it->m_pos = pimpl->m_pos + offs;
  it->m_stream = pimpl->m_stream;
  it->m_limit = pimpl->m_limit;

  return iter(std::move(it));
}

iter::iterator(std::unique_ptr<riffcpp::Chunk::iterator::impl> &&impl) {
  pimpl = std::move(impl);
}

bool iter::operator==(const iter &a) const {
  return pimpl->m_pos == a.pimpl->m_pos;
}
bool iter::operator!=(const iter &a) const { return !(*this == a); }

riffcpp::Chunk iter::operator*() const {
  auto im = std::make_unique<riffcpp::Chunk::impl>();
  im->m_stream = pimpl->m_stream;
  im->m_pos = pimpl->m_pos;

  im->m_id = read_id(im->m_stream, im->m_pos);
  im->m_size = read_size(im->m_stream, im->m_pos);

  auto size_id = read_size_as_id(im->m_stream, im->m_pos);

  auto limit = im->m_pos + std::streamoff{im->m_size};

  if (limit > pimpl->m_limit) {
    throw Error("Chunk size outside of range", ErrorType::InvalidFile);
  }

  im->m_limit = limit;

  return riffcpp::Chunk(std::move(im));
}

iter &iter::operator++() {
  auto im = std::make_unique<riffcpp::Chunk::impl>();
  im->m_stream = pimpl->m_stream;
  im->m_pos = pimpl->m_pos;

  im->m_id = read_id(im->m_stream, im->m_pos);
  im->m_size = read_size(im->m_stream, im->m_pos);

  auto limit = im->m_pos + std::streamoff{im->m_size};

  if (limit > pimpl->m_limit) {
    throw Error("Chunk size outside of range", ErrorType::InvalidFile);
  }
  im->m_limit = limit;

  riffcpp::Chunk chunk(std::move(im));
  std::uint32_t sz = chunk.size();
  std::streamoff offs{sz + sz % 2 + 8};

  pimpl->m_pos += offs;
  return *this;
}

iter iter::operator++(int) {
  auto im = std::make_unique<riffcpp::Chunk::iterator::impl>();
  im->m_stream = pimpl->m_stream;
  im->m_pos = pimpl->m_pos;
  im->m_limit = pimpl->m_limit;

  this->operator++();
  return iter(std::move(im));
}

riffcpp::Chunk::iterator::~iterator() = default;

riffcpp::Chunk::iterator::iterator(const iter &it) {
  auto im = std::make_unique<riffcpp::Chunk::iterator::impl>();
  im->m_stream = it.pimpl->m_stream;
  im->m_pos = it.pimpl->m_pos;
  im->m_limit = it.pimpl->m_limit;

  pimpl = std::move(im);
}