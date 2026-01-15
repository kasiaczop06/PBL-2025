#!/usr/bin/env python3
"""
HX711 Scale Calibration Tool
This script helps you calibrate your HX711 load cell
by calculating the calibration factor based on known weights.
"""

import sys

def calculate_calibration():
    print("=" * 60)
    print("HX711 Scale Calibration Calculator")
    print("=" * 60)
    print()

    print("Instructions:")
    print("1. Place no weight on the scale")
    print("2. Flash the ESP32 with default calibration (1.0)")
    print("3. Read the 'raw value' from serial monitor")
    print("4. Place a known weight on the scale")
    print("5. Read the new 'raw value' from serial monitor")
    print()

    try:
        # Get raw value without weight
        raw_no_weight = float(input("Enter raw value with NO weight: "))

        # Get raw value with known weight
        raw_with_weight = float(input("Enter raw value WITH known weight: "))

        # Get known weight
        known_weight = float(input("Enter the known weight (in grams or your preferred unit): "))

        # Calculate calibration factor
        raw_difference = raw_with_weight - raw_no_weight

        if raw_difference == 0:
            print("\nError: No change in raw value detected!")
            print("Make sure you placed weight on the scale.")
            return

        calibration_factor = raw_difference / known_weight

        # Display results
        print()
        print("=" * 60)
        print("CALIBRATION RESULTS")
        print("=" * 60)
        print(f"Raw value (no weight):    {raw_no_weight:.2f}")
        print(f"Raw value (with weight):  {raw_with_weight:.2f}")
        print(f"Raw difference:           {raw_difference:.2f}")
        print(f"Known weight:             {known_weight:.2f}")
        print()
        print(f"CALIBRATION FACTOR:       {calibration_factor:.2f}")
        print("=" * 60)
        print()

        # Provide instructions for updating code
        print("To use this calibration factor:")
        print("1. Open main/clock_idf.c")
        print("2. Find the line: hx711_set_scale(&scale, -16347.25);")
        print(f"3. Replace with: hx711_set_scale(&scale, {calibration_factor:.2f});")
        print("4. Rebuild and flash: idf.py build flash")
        print()

        # Test calculation
        print("Verification:")
        test_weight = known_weight / 2
        expected_raw = raw_no_weight + (calibration_factor * test_weight)
        print(f"If you place {test_weight:.2f} units on the scale,")
        print(f"you should see a raw value of approximately {expected_raw:.2f}")
        print()

    except ValueError:
        print("\nError: Please enter valid numbers!")
        return
    except KeyboardInterrupt:
        print("\n\nCalibration cancelled.")
        return

def estimate_coin_weight():
    print()
    print("=" * 60)
    print("Would you like to estimate coin detection range? (y/n): ", end="")
    response = input().lower()

    if response == 'y':
        print()
        print("Common coin weights (reference):")
        print("  5 PLN coin:     6.54 g")
        print("  2 PLN coin:     8.15 g")
        print("  1 PLN coin:     5.00 g")
        print("  US Quarter:     5.67 g")
        print("  US Dime:        2.27 g")
        print()

        try:
            coin_weight = float(input("Enter your coin weight (in grams): "))
            tolerance = float(input("Enter tolerance percentage (e.g., 10 for ±10%): "))

            min_weight = coin_weight * (1 - tolerance / 100)
            max_weight = coin_weight * (1 + tolerance / 100)

            print()
            print(f"Coin detection range: {min_weight:.2f}g to {max_weight:.2f}g")
            print()
            print("Update these values in main/clock_idf.c:")
            print(f"  - Line ~295: if(temp_i>={min_weight:.2f})")
            print(f"  - Line ~312: if(temp_i2>={min_weight:.2f} && temp_i2<={max_weight:.2f})")
            print()

        except ValueError:
            print("Invalid input!")

def main():
    print()
    calculate_calibration()
    estimate_coin_weight()
    print()
    print("=" * 60)
    print("Calibration complete!")
    print("=" * 60)
    print()

if __name__ == "__main__":
    main()
