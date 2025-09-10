namespace ApcMonitorCore.Managed.Console;

internal class Program
{
    static void Main(string[] args)
    {
        try
        {
            // Create an instance of the managed monitor service
            using var service = new ManagedMonitorService();

            // Get monitor data
            List<ManagedMonitorData> monitors = service.GetMonitorsData();

            // Display information about each monitor
            System.Console.WriteLine($"Found {monitors.Count} monitor(s):\n");

            for (int i = 0; i < monitors.Count; i++)
            {
                var monitor = monitors[i];

                System.Console.WriteLine($"Monitor {i + 1}:");
                System.Console.WriteLine($"  ID: {monitor.Id}");
                System.Console.WriteLine($"  Primary: {(monitor.IsPrimary ? "Yes" : "No")}");
                System.Console.WriteLine($"  Friendly Name: {monitor.FriendlyName}");
                System.Console.WriteLine($"  Device Name: {monitor.DeviceName}");
                System.Console.WriteLine($"  Serial Number: {monitor.SerialNumber}");
                System.Console.WriteLine($"  Key: {monitor.Key}");
                System.Console.WriteLine($"  Position: {monitor.RelativePosition}");
                System.Console.WriteLine($"  Display Name: {monitor.DisplayName}");
                System.Console.WriteLine($"  Monitor Rect: {monitor.MonitorRect}");
                System.Console.WriteLine($"  Work Rect: {monitor.WorkRect}");
                System.Console.WriteLine();
            }

            // Example: Find a specific monitor
            if (monitors.Count > 0)
            {
                var firstMonitor = monitors[0];
                int index = ManagedMonitorService.FindMonitorIndex(
                    monitors,
                    firstMonitor.Key,
                    firstMonitor.MonitorRect);
                System.Console.WriteLine($"Successfully found monitor at index: {index}");
            }

            // Example: Use custom display name format
            if (monitors.Count > 0)
            {
                var customDisplayName = monitors[0].GetDisplayName("{FriendlyName} - {SerialNumber}");
                System.Console.WriteLine($"Custom display name: {customDisplayName}");
            }
        }
        catch (Exception ex)
        {
            System.Console.WriteLine($"Error: {ex.Message}");
            return;
        }

        System.Console.WriteLine("Press any key to exit...");
        System.Console.ReadKey();
    }
}