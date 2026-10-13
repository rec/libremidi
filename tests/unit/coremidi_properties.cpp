#include "../include_catch.hpp"
#include <libremidi/backends/coremidi/observer.hpp>

#include <limits>

// This executable supplies the property getter, leaving its output untouched on
// failure. No MIDI clients or endpoints are created and no device is queried.
namespace
{
OSStatus property_status = noErr;
SInt32 property_value{};
}

extern "C" OSStatus
MIDIObjectGetIntegerProperty(MIDIObjectRef, CFStringRef, SInt32* out_value)
{
  if (property_status == noErr)
    *out_value = property_value;
  return property_status;
}

TEST_CASE("CoreMIDI integer properties preserve successful values", "[coremidi]")
{
  property_status = noErr;
  for (const SInt32 value : {0, 1, -1, std::numeric_limits<SInt32>::min(),
                            std::numeric_limits<SInt32>::max()})
  {
    property_value = value;
    REQUIRE(libremidi::get_int_property(0, kMIDIPropertyUniqueID) == value);
  }
}

TEST_CASE("CoreMIDI failed property reads produce no port ID", "[coremidi]")
{
  property_status = kMIDIObjectNotFound;
  property_value = 123; // The failing getter deliberately does not write this.
  REQUIRE_FALSE(libremidi::get_int_property(0, kMIDIPropertyUniqueID).has_value());

  int errors{};
  std::string last_error;
  libremidi::observer_core observer{
      libremidi::observer_configuration{
          .on_error = [&](std::string_view text, const libremidi::source_location&) {
            ++errors;
            last_error = text;
          }},
      libremidi::coremidi_observer_configuration{}};
  REQUIRE_FALSE(observer.to_port_info<true>(0).has_value());
  REQUIRE_FALSE(observer.to_port_info<false>(0).has_value());
  REQUIRE(errors == 2);
  REQUIRE(last_error == "could not read CoreMIDI port unique ID");
}
