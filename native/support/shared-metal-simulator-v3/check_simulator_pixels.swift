import AppKit
import Foundation

guard CommandLine.arguments.count == 2,
      let data = try? Data(contentsOf: URL(fileURLWithPath: CommandLine.arguments[1])),
      let image = NSBitmapImageRep(data: data),
      image.pixelsWide >= 320, image.pixelsHigh >= 600 else {
    fputs("Missing or malformed simulator screenshot\n", stderr)
    exit(2)
}

func sample(_ xFraction: Double, _ yFraction: Double, green: Bool) -> Bool {
    let x = Int(Double(image.pixelsWide) * xFraction)
    let y = Int(Double(image.pixelsHigh) * yFraction)
    for dy in -2...2 {
        for dx in -2...2 {
            guard let color = image.colorAt(x: x + dx, y: y + dy)?.usingColorSpace(NSColorSpace.deviceRGB),
                  color.alphaComponent > 0.9, color.redComponent < 0.20 else { return false }
            if green {
                guard color.greenComponent > 0.80, color.blueComponent < 0.20 else { return false }
            } else {
                guard color.blueComponent > 0.80, color.greenComponent < 0.20 else { return false }
            }
        }
    }
    return true
}

guard sample(0.08, 0.50, green: false), sample(0.50, 0.50, green: true) else {
    fputs("Simulator screenshot samples differ from the expected blue and green\n", stderr)
    exit(3)
}
print("PASS blue side and green center in the actual simulator screenshot")
