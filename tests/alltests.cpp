#define CATCH_CONFIG_MAIN
#include "catch2.hpp"

#include "Tunings.h"
#include <iomanip>
#include <vector>

/*
** ToDo
** tuning with non-contiguous kbm
** tuning with non-monotonic kbm
** few known tunings across the whole spectrun
*/

std::string testFile(std::string fn) {
    return std::string( "tests/data/" ) + fn;
}

std::vector<std::string> testSCLs() {
    std::vector<std::string> res = { {
            "12-intune.scl", 
            "12-shuffled.scl" ,
            "31edo.scl", 
            "6-exact.scl" ,
            "marvel12.scl" ,
            "zeus22.scl",
            "ED4-17.scl",
            "ED3-17.scl"
        } }; 
    return res;
        
}

std::vector<std::string> testKBMs() {
    std::vector<std::string> res = { {
            "empty-note61.kbm",
            "empty-note69.kbm",
            "mapping-a440-constant.kbm",
            "mapping-a442-7-to-12.kbm",
            "mapping-whitekeys-a440.kbm",
            "mapping-whitekeys-c261.kbm",
            "shuffle-a440-constant.kbm"
        } };
    return res;
}

TEST_CASE( "Loading tuning files" )
{
    SECTION( "Load a 12 tone standard tuning" )
    {
        auto s = Tunings::readSCLFile( testFile( "12-intune.scl" ) );
        REQUIRE( s.count == 12 );
        // FIXME - write a lot more here obviously
    }

    SECTION( "KBM File from text" )
    {
        std::ostringstream oss;
        oss << "! A scale file\n"
            << "! with zero size\n"
            << "0\n"
            << "! spanning the keybaord\n"
            << "0\n"
            << "127\n"
            << "! With C60 as constant and A as 452\n"
            << "60\n69\n452\n"
            << "! and an octave might as well be zero\n"
            << "0\n";

        REQUIRE_NOTHROW( Tunings::parseKBMData( oss.str() ) );
    }
}


TEST_CASE( "Identity Tuning Tests" )
{
    SECTION( "12-intune tunes properly" )
    {
        auto s = Tunings::readSCLFile( testFile( "12-intune.scl" ) );
        REQUIRE( s.count == 12 );
        Tunings::Tuning t( s );
        REQUIRE( t.frequencyForMidiNote( 69 ) == Approx( 440.0 ).margin( 1e-10 ) );
        REQUIRE( t.frequencyForMidiNoteScaledByMidi0( 60 ) == 32.0 );
        REQUIRE( t.logScaledFrequencyForMidiNote( 60 ) == 5.0 );
    }

    SECTION( "12-intune doubles properly" )
    {
        auto s = Tunings::readSCLFile( testFile( "12-intune.scl" ) );
        Tunings::Tuning t( s );
        for( int i=0; i<12; ++i )
        {
            int note = - 12 * 4 + i;
            auto sc = t.frequencyForMidiNoteScaledByMidi0( note );
            auto lc = t.logScaledFrequencyForMidiNote( note );
            while( note < 200 )
            {
                note += 12;
                auto nlc = t.logScaledFrequencyForMidiNote( note );
                auto nsc = t.frequencyForMidiNoteScaledByMidi0( note );
                REQUIRE( nsc == Approx( sc * 2 ).margin(1e-8) );
                REQUIRE( nlc == Approx(lc + 1).margin(1e-8) );
                sc = nsc;
                lc = nlc;
            }
        }
    }

    SECTION( "Scaling is constant" )
    {
        auto s = Tunings::readSCLFile( testFile( "12-intune.scl" ) );
        Tunings::Tuning t( s );
        auto f60 = t.frequencyForMidiNote( 60 );
        auto fs60 = t.frequencyForMidiNoteScaledByMidi0( 60 );
        for( int i=-200; i<200; ++i )
        {
            auto f = t.frequencyForMidiNote( i );
            auto fs = t.frequencyForMidiNoteScaledByMidi0( i );
            REQUIRE( f/fs == f60/fs60 );
        }
    }
}

TEST_CASE( "Simple Keyboard Remapping Tunes A69" )
{
    SECTION( "A440" )
    {
        auto k = Tunings::tuneA69To( 440.0 );
        Tunings::Tuning t(k);
        REQUIRE( t.frequencyForMidiNote( 69 ) == Approx( 440.0 ).margin( 1e-10 ) );
        REQUIRE( t.frequencyForMidiNote( 60 ) == Approx( 261.625565301 ).margin( 1e-10 ) );
    }

    SECTION( "A432" )
    {
        auto k = Tunings::tuneA69To( 432.0 );
        Tunings::Tuning t(k);
        REQUIRE( t.frequencyForMidiNote( 69 ) == Approx( 432.0 ).margin( 1e-10 ) );
        REQUIRE( t.frequencyForMidiNote( 60 ) == Approx( 261.625565301  * 432 / 440).margin( 1e-10 ) );
    }

    SECTION( "Random As Scale Consistently" )
    {
        Tunings::Tuning ut;
        
        for( int i=0; i<100; ++i )
        {
            auto fr = 400 + 80.0 * rand() / RAND_MAX;
            
            auto k = Tunings::tuneA69To( fr );
            Tunings::Tuning t(k);
            REQUIRE( t.frequencyForMidiNote( 69 ) == Approx( fr ).margin( 1e-10 ) );
            REQUIRE( t.frequencyForMidiNote( 60 ) == Approx( 261.625565301  * fr / 440).margin( 1e-10 ) );

            double ldiff = t.logScaledFrequencyForMidiNote(69) - ut.logScaledFrequencyForMidiNote(69);
            double ratio = t.frequencyForMidiNote(69) / ut.frequencyForMidiNote(69);

            for( int j=-200; j<200; ++j )
            {
                REQUIRE( t.logScaledFrequencyForMidiNote(j) - ut.logScaledFrequencyForMidiNote(j) == Approx( ldiff ).margin( 1e-8 ) );
                REQUIRE( t.frequencyForMidiNote(69) / ut.frequencyForMidiNote(69) == Approx( ratio ).margin( 1e-8 ) );
            }
        }

    }
}

TEST_CASE( "Internal Constraints between Measures" )
{
    SECTION( "Test All Constraints SCL only" )
    {
        for( auto f : testSCLs() )
        {
            INFO( "Testing Constraints with " << f );
            auto s = Tunings::readSCLFile(testFile(f));
            Tunings::Tuning t(s);
            
            for( int i=0; i<127; ++i )
            {
                REQUIRE( t.frequencyForMidiNote(i) == t.frequencyForMidiNoteScaledByMidi0(i) * Tunings::MIDI_0_FREQ );
                REQUIRE( t.frequencyForMidiNoteScaledByMidi0(i) == pow( 2.0, t.logScaledFrequencyForMidiNote(i) ) );
            }
        }
    }

    SECTION( "Test All Constraints KBM only" )
    {
        for( auto f : testKBMs() )
        {
            INFO( "Testing Constraints with " << f );
            auto k = Tunings::readKBMFile(testFile(f));
            Tunings::Tuning t(k);
            
            for( int i=0; i<127; ++i )
            {
                REQUIRE( t.frequencyForMidiNote(i) == t.frequencyForMidiNoteScaledByMidi0(i) * Tunings::MIDI_0_FREQ );
                REQUIRE( t.frequencyForMidiNoteScaledByMidi0(i) == pow( 2.0, t.logScaledFrequencyForMidiNote(i) ) );
            }
        }
    }

    SECTION( "Test All Constraints SCL and KBM" )
    {
        for( auto fs: testSCLs() )
            for( auto fk : testKBMs() )
            {
                INFO( "Testing Constraints with " << fs << " " << fk );
                auto s = Tunings::readSCLFile(testFile(fs));
                auto k = Tunings::readKBMFile(testFile(fk));
                Tunings::Tuning t(s,k);
                
                for( int i=0; i<127; ++i )
                {
                    REQUIRE( t.frequencyForMidiNote(i) == t.frequencyForMidiNoteScaledByMidi0(i) * Tunings::MIDI_0_FREQ );
                    REQUIRE( t.frequencyForMidiNoteScaledByMidi0(i) == pow( 2.0, t.logScaledFrequencyForMidiNote(i) ) );
                }
            }
    }
}

TEST_CASE( "Several Sample Scales" )
{
    SECTION( "Non Monotonic 12 note" )
    {
        auto s = Tunings::readSCLFile( testFile( "12-shuffled.scl" ) );
        Tunings::Tuning t(s);
        REQUIRE( s.count == 12 );
        REQUIRE( t.logScaledFrequencyForMidiNote(60) == 5 );
        
        std::vector<int> order = { { 0, 2, 1, 3, 5, 4, 6, 7, 8, 10, 9, 11, 12 } };
        auto l60 = t.logScaledFrequencyForMidiNote(60);
        for( size_t i=0; i<order.size(); ++i )
        {
            auto li = t.logScaledFrequencyForMidiNote(60 + i);
            auto oi = order[i];
            REQUIRE( li - l60 == Approx( oi / 12.0 ).margin( 1e-6 ) );
        }
    }

    SECTION( "31 edo" )
    {
        auto s = Tunings::readSCLFile( testFile( "31edo.scl" ) );
        Tunings::Tuning t(s);
        REQUIRE( s.count == 31 );
        REQUIRE( t.logScaledFrequencyForMidiNote(60) == 5 );

        auto prev = t.logScaledFrequencyForMidiNote(60);
        for( int i=1; i<31; ++i )
        {
            auto curr = t.logScaledFrequencyForMidiNote(60 + i);
            REQUIRE( curr - prev == Approx( 1.0 / 31.0 ).margin( 1e-6 ) );
            prev = curr;
        }
    }

    SECTION( "ED3-17" )
    {
        auto s = Tunings::readSCLFile( testFile( "ED3-17.scl" ) );
        Tunings::Tuning t(s);
        REQUIRE( s.count == 17 );
        REQUIRE( t.logScaledFrequencyForMidiNote(60) == 5 );

        auto prev = t.logScaledFrequencyForMidiNote(60);
        for( int i=1; i<40; ++i )
        {
            auto curr = t.logScaledFrequencyForMidiNote(60 + i);
            REQUIRE( pow( 2.0, 17*(curr - prev) ) == Approx( 3.0 ).margin( 1e-6 ) );
            prev = curr;
        }
    }

    SECTION( "ED4-17" )
    {
        auto s = Tunings::readSCLFile( testFile( "ED4-17.scl" ) );
        Tunings::Tuning t(s);
        REQUIRE( s.count == 17 );
        REQUIRE( t.logScaledFrequencyForMidiNote(60) == 5 );

        auto prev = t.logScaledFrequencyForMidiNote(60);
        for( int i=1; i<40; ++i )
        {
            auto curr = t.logScaledFrequencyForMidiNote(60 + i);
            REQUIRE( pow( 2.0, 17*(curr - prev) ) == Approx( 4.0 ).margin( 1e-6 ) );
            prev = curr;
        }
    }

    SECTION( "6 exact" )
    {
        auto s = Tunings::readSCLFile( testFile( "6-exact.scl" ) );
        Tunings::Tuning t(s);
        REQUIRE( s.count == 6 );
        REQUIRE( t.logScaledFrequencyForMidiNote(60) == 5 );

        std::vector<double> knownValues = { { 0, 0.22239,  0.41504, 0.58496, 0.73697, 0.87447, 1.0 } };
        
        for( size_t i=0; i<knownValues.size(); ++i )
            REQUIRE( t.logScaledFrequencyForMidiNote( 60 + i ) ==
                     Approx( t.logScaledFrequencyForMidiNote( 60 ) + knownValues[i] ).margin( 1e-5 ) );
    }
}

TEST_CASE( "Remapping frequency with non-12-length scales" )
{
    SECTION( "6 exact" )
    {
        auto s = Tunings::readSCLFile( testFile( "6-exact.scl" ) );
        Tunings::Tuning t(s);

        for( int i=0; i<100; ++i )
        {
            int mn = rand() % 40 + 40;
            double freq = 150 + 300.0 * rand() / RAND_MAX;
            INFO( "Setting " << mn << " to " << freq );
            auto k = Tunings::tuneNoteTo( mn, freq );
            Tunings::Tuning mapped(s,k);

            REQUIRE( mapped.frequencyForMidiNote( mn ) == Approx( freq ).margin( 1e-6 ) );

            // This scale is monotonic so test monotonicity still
            for( int i=1; i<127; ++i )
            {
                INFO( "About to test at " << i );
                if( mapped.frequencyForMidiNote(i) > 1 )
                    REQUIRE(  mapped.frequencyForMidiNote( i ) > mapped.frequencyForMidiNote( i - 1 ) );
            }

            double n60ldiff = t.logScaledFrequencyForMidiNote(60) - mapped.logScaledFrequencyForMidiNote(60);
            for( int j=0; j<128; ++j )
            {
                REQUIRE( t.logScaledFrequencyForMidiNote(j) - mapped.logScaledFrequencyForMidiNote(j) ==
                         Approx( n60ldiff ).margin( 1e-6 ) ); 
            }
        }
    }

    SECTION( "31 edo" )
    {
        auto s = Tunings::readSCLFile( testFile( "31edo.scl" ) );
        Tunings::Tuning t(s);

        for( int i=0; i<100; ++i )
        {
            int mn = rand() % 20 + 50;
            double freq = 150 + 300.0 * rand() / RAND_MAX;
            INFO( "Setting " << mn << " to " << freq );
            auto k = Tunings::tuneNoteTo( mn, freq );
            Tunings::Tuning mapped(s,k);

            REQUIRE( mapped.frequencyForMidiNote( mn ) == Approx( freq ).margin( 1e-6 ) );

            // This scale is monotonic so test monotonicity still
            for( int i=1; i<127; ++i )
            {
                INFO( "About to test at " << i );
                if( mapped.frequencyForMidiNote(i) > 1 )
                    REQUIRE(  mapped.frequencyForMidiNote( i ) > mapped.frequencyForMidiNote( i - 1 ) );
            }

            double n60ldiff = t.logScaledFrequencyForMidiNote(60) - mapped.logScaledFrequencyForMidiNote(60);
            for( int j=0; j<128; ++j )
            {
                REQUIRE( t.logScaledFrequencyForMidiNote(j) - mapped.logScaledFrequencyForMidiNote(j) ==
                         Approx( n60ldiff ).margin( 1e-6 ) ); 
            }
        }

    }

    SECTION( "ED4-17" )
    {
        auto s = Tunings::readSCLFile( testFile( "ED4-17.scl" ) );
        Tunings::Tuning t(s);

        for( int i=0; i<100; ++i )
        {
            int mn = rand() % 40 + 40;
            double freq = 150 + 300.0 * rand() / RAND_MAX;
            INFO( "Setting " << mn << " to " << freq );
            auto k = Tunings::tuneNoteTo( mn, freq );
            Tunings::Tuning mapped(s,k);

            REQUIRE( mapped.frequencyForMidiNote( mn ) == Approx( freq ).margin( 1e-6 ) );

            // This scale is monotonic so test monotonicity still
            for( int i=1; i<127; ++i )
            {
                INFO( "About to test at " << i );
                if( mapped.frequencyForMidiNote(i) > 1 )
                    REQUIRE(  mapped.frequencyForMidiNote( i ) > mapped.frequencyForMidiNote( i - 1 ) );
            }

            double n60ldiff = t.logScaledFrequencyForMidiNote(60) - mapped.logScaledFrequencyForMidiNote(60);
            for( int j=0; j<128; ++j )
            {
                REQUIRE( t.logScaledFrequencyForMidiNote(j) - mapped.logScaledFrequencyForMidiNote(j) ==
                         Approx( n60ldiff ).margin( 1e-6 ) ); 
            }
        }
    }

    SECTION( "ED3-17" )
    {
        auto s = Tunings::readSCLFile( testFile( "ED3-17.scl" ) );
        Tunings::Tuning t(s);

        for( int i=0; i<100; ++i )
        {
            int mn = rand() % 40 + 40;
            double freq = 150 + 300.0 * rand() / RAND_MAX;
            INFO( "Setting " << mn << " to " << freq );
            auto k = Tunings::tuneNoteTo( mn, freq );
            Tunings::Tuning mapped(s,k);

            REQUIRE( mapped.frequencyForMidiNote( mn ) == Approx( freq ).margin( 1e-6 ) );

            // This scale is monotonic so test monotonicity still
            for( int i=1; i<127; ++i )
            {
                INFO( "About to test at " << i );
                if( mapped.frequencyForMidiNote(i) > 1 )
                    REQUIRE(  mapped.frequencyForMidiNote( i ) > mapped.frequencyForMidiNote( i - 1 ) );
            }

            double n60ldiff = t.logScaledFrequencyForMidiNote(60) - mapped.logScaledFrequencyForMidiNote(60);
            for( int j=0; j<128; ++j )
            {
                REQUIRE( t.logScaledFrequencyForMidiNote(j) - mapped.logScaledFrequencyForMidiNote(j) ==
                         Approx( n60ldiff ).margin( 1e-6 ) ); 
            }
        }
    }
}

TEST_CASE( "KBMs with Gaps" )
{
    SECTION( "12 Intune with Gap" )
    {
        auto s = Tunings::readSCLFile( testFile( "12-intune.scl" ) );
        auto k = Tunings::readKBMFile( testFile( "mapping-whitekeys-c261.kbm" ) );
        Tunings::Tuning t( s );
        Tunings::Tuning tm(s, k);

        REQUIRE( s.count == 12 );
        REQUIRE( t.frequencyForMidiNote(69) == Approx( 440.0 ).margin(1e-6) );

        // That KBM maps the white keys to the chromatic start so
        std::vector<std::pair<int, int> > maps = { {60, 60},
                                                   {61, 62},
                                                   {62, 64},
                                                   {63, 65},
                                                   {64, 67},
                                                   {65, 69},
                                                   {66, 71} };
        for( auto p : maps )
        {
            REQUIRE( t.logScaledFrequencyForMidiNote(p.first) == Approx( tm.logScaledFrequencyForMidiNote(p.second) ).margin(1e-5) );
        }
    }
}

TEST_CASE( "KBM ReOrdering" )
{
    SECTION( "Non Monotonic KBM note" )
    {
        auto s = Tunings::readSCLFile( testFile( "12-intune.scl" ) );
        auto k = Tunings::readKBMFile( testFile( "shuffle-a440-constant.kbm" ) );
        Tunings::Tuning t(s, k);

        REQUIRE( s.count == 12 );
        REQUIRE( t.frequencyForMidiNote(69) == Approx( 440.0 ).margin(1e-6) );

        std::vector<int> order = { { 0, 2, 1, 3, 4, 6, 5, 7, 8, 9, 11, 10, 12 } };
        auto l60 = t.logScaledFrequencyForMidiNote(60);
        for( size_t i=0; i<order.size(); ++i )
        {
            INFO( "Testing note " << i );
            auto li = t.logScaledFrequencyForMidiNote(60 + i);
            auto oi = order[i];
            REQUIRE( li - l60 == Approx( oi / 12.0 ).margin( 1e-6 ) );
        }
    }
}

TEST_CASE( "Exceptions and Bad Files" )
{
    SECTION( "Read Non-present files" )
    {
        REQUIRE_THROWS( Tunings::readSCLFile( "blahlfdsfds" ) );
        REQUIRE_THROWS( Tunings::readKBMFile( "blahlfdsfds" ) );

        // Lets make sure what is reasonable
        try {
            Tunings::readSCLFile( "MISSING" );
        }
        catch( const Tunings::TuningError &e )
        {
            REQUIRE( std::string( e.what() ) == "Unable to open file 'MISSING'" );
        }
    }

    SECTION( "Bad SCL" )
    {
        // Trailing data is OK
        REQUIRE_NOTHROW( Tunings::readSCLFile( testFile( "bad/extraline.scl" ) ) );
        

        REQUIRE_THROWS( Tunings::readSCLFile( testFile( "bad/badnote.scl" ) ) );
        REQUIRE_THROWS( Tunings::readSCLFile( testFile( "bad/blanknote.scl" ) ) );
        REQUIRE_THROWS( Tunings::readSCLFile( testFile( "bad/missingnote.scl" ) ) );
    }

    SECTION( "Bad KBM" )
    {
        REQUIRE_THROWS( Tunings::readKBMFile( testFile( "bad/blank-line.kbm" ) ) );
        REQUIRE_THROWS( Tunings::readKBMFile( testFile( "bad/empty-bad.kbm" ) ) );
        REQUIRE_THROWS( Tunings::readKBMFile( testFile( "bad/garbage-key.kbm" ) ) );
        REQUIRE_NOTHROW( Tunings::readKBMFile( testFile( "bad/empty-extra.kbm" ) ) );
        REQUIRE_NOTHROW( Tunings::readKBMFile( testFile( "bad/extraline-long.kbm" ) ) );
        REQUIRE_THROWS( Tunings::readKBMFile( testFile( "bad/missing-note.kbm" ) ) );
    }
}

TEST_CASE( "EDN-M" )
{
    SECTION( "ED2" )
    {
        auto s = Tunings::evenDivisionOfSpanByM( 2, 12 );
        REQUIRE( s.count == 12 );
        Tunings::Tuning ut;
        Tunings::Tuning t(s);
        for( int i=0;i<128;++i )
            REQUIRE( t.logScaledFrequencyForMidiNote(i) == ut.logScaledFrequencyForMidiNote(i) );
    }

    SECTION( "ED3-17" )
    {
        auto s = Tunings::evenDivisionOfSpanByM( 3, 17 );
        auto sf = Tunings::readSCLFile( testFile( "ED3-17.scl" ) );

        Tunings::Tuning ut(sf);
        Tunings::Tuning t(s);
        for( int i=0;i<128;++i )
            REQUIRE( t.logScaledFrequencyForMidiNote(i) == Approx( ut.logScaledFrequencyForMidiNote(i) ).margin( 1e-6 ));
    }

    SECTION( "ED4-17" )
    {
        auto s = Tunings::evenDivisionOfSpanByM( 4, 17 );
        auto sf = Tunings::readSCLFile( testFile( "ED4-17.scl" ) );

        Tunings::Tuning ut(sf);
        Tunings::Tuning t(s);
        for( int i=0;i<128;++i )
            REQUIRE( t.logScaledFrequencyForMidiNote(i) == Approx( ut.logScaledFrequencyForMidiNote(i) ).margin(1e-6) );
    }

    SECTION( "Constraints on random EDN-M" )
    {
        for( int i=0; i<100; ++i )
        {
            int Span = rand() % 7 + 2;
            int M = rand() % 50 + 3;
            INFO( "Constructing " << i << " scale ED " << Span << " - " << M );

            auto s = Tunings::evenDivisionOfSpanByM( Span, M );

            REQUIRE( s.count == M );

            Tunings::Tuning t(s);
            REQUIRE( t.frequencyForMidiNoteScaledByMidi0(60) * Span ==
                     Approx( t.frequencyForMidiNoteScaledByMidi0(60+M) ).margin( 1e-7 ) );

            auto d0 = t.logScaledFrequencyForMidiNote(1) - t.logScaledFrequencyForMidiNote(0);
            for( auto i=1; i<128; ++i )
            {
                auto d = t.logScaledFrequencyForMidiNote(i) - t.logScaledFrequencyForMidiNote(i-1);
                REQUIRE( d == Approx( d0 ).margin( 1e-7 ) );
            }
        }
    }
}

