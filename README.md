# fastcgipp
FastCGI in c++

## Requirements
A c++14 compiler.

## Usage
For decoding the stream of fastcgi frames you need a handler for the FastCGI::Decoder class. A minimalistic version of a handler class would look like this:
```
class MyHandler
{
    template<FastCGI::Type TYPE, class... ARGS>
    bool operator()(FastCGI::Atom<TYPE>, const FastCGI::RecordHeader& header, ARGS...)
    {
        // returning false prevents the FastCGI::Decoder to continue decoding the rest of the stream
        return true;
    }
    // When an unknown type is inside the stream
    bool operator()(const FastCGI::RecordHeader& header, const char* payload, const uint16_t length)
    {
        return true;
    }
};
```

## Author
* Mohsen Khaki

## License

This project is licensed under the MIT License - see the LICENSE file for details.
