using System.IO.Ports;
using System.Management;
using System.Runtime.Versioning;


/// <summary>
/// Helper methods for COM port management
/// </summary>
public static class ComPortHelper
{
    /// <summary>
    /// Lists all available COM ports
    /// </summary>
    public static string[] GetAvailablePorts()
    {
        return SerialPort.GetPortNames();
    }

    /// <summary>
    /// Checks if a specific COM port is available
    /// </summary>
    public static bool IsPortAvailable(string portName)
    {
        return GetAvailablePorts().Contains(portName, StringComparer.OrdinalIgnoreCase);
    }

    /// <summary>
    /// Attempts to open a port to test if it is accessible
    /// </summary>
    public static bool CanAccessPort(string portName, int baudRate = 115200)
    {
        try
        {
            using var port = new SerialPort(portName, baudRate);
            port.Open();
            port.Close();
            return true;
        }
        catch
        {
            return false;
        }
    }

    /// <summary>
    /// Displays all available COM ports in the console
    /// </summary>
    public static void DisplayAvailablePorts()
    {
        var ports = GetAvailablePorts();
        
        if (ports.Length == 0)
        {
            Console.WriteLine("No COM ports found!");
            return;
        }

        Console.WriteLine("Available COM ports:");
        Console.WriteLine("=====================");
        
        foreach (var port in ports)
        {
            var accessible = CanAccessPort(port) ? "✓ Accessible" : "✗ Blocked";
            var description = GetPortDescription(port);
            
            Console.WriteLine($"  {port,-8} {accessible,-15} {description}");
        }
        
        Console.WriteLine();
    }

    /// <summary>
    /// Lets the user interactively select a COM port
    /// </summary>
    public static string? SelectPortInteractive()
    {
        var ports = GetAvailablePorts();
        
        if (ports.Length == 0)
        {
            Console.WriteLine("No COM ports found!");
            return null;
        }

        Console.WriteLine("Available COM ports:");
        for (int i = 0; i < ports.Length; i++)
        {
            var accessible = CanAccessPort(ports[i]) ? "✓" : "✗";
            var description = GetPortDescription(ports[i]);
            Console.WriteLine($"  [{i + 1}] {ports[i]} {accessible} - {description}");
        }

        Console.WriteLine();
        Console.Write("Select a port (1-{0}) or press [Enter] for default: ", ports.Length);
        
        var input = Console.ReadLine();
        
        if (string.IsNullOrWhiteSpace(input))
        {
            return null;
        }

        if (int.TryParse(input, out int selection) && selection >= 1 && selection <= ports.Length)
        {
            return ports[selection - 1];
        }

        Console.WriteLine("Invalid selection!");
        return null;
    }

    /// <summary>
    /// Attempts to retrieve a description for the COM port (Windows only)
    /// </summary>
    [SupportedOSPlatform("windows")]
    private static string GetPortDescriptionWindows(string portName)
    {
        try
        {
            using var searcher = new ManagementObjectSearcher(
                "SELECT * FROM Win32_PnPEntity WHERE Caption LIKE '%(COM%'");
            
            foreach (ManagementObject obj in searcher.Get())
            {
                string? caption = obj["Caption"]?.ToString();
                if (caption != null && caption.Contains(portName, StringComparison.OrdinalIgnoreCase))
                {
                    return caption;
                }
            }
        }
        catch
        {
            // Ignore errors
        }

        return "Unknown device";
    }

    private static string GetPortDescription(string portName)
    {
        if (OperatingSystem.IsWindows())
        {
            return GetPortDescriptionWindows(portName);
        }
        
        return "COM port";
    }

    /// <summary>
    /// Finds the first available and accessible COM port
    /// </summary>
    public static string? FindFirstAccessiblePort()
    {
        var ports = GetAvailablePorts();
        
        foreach (var port in ports)
        {
            if (CanAccessPort(port))
            {
                return port;
            }
        }

        return null;
    }
}