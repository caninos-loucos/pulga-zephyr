# Building an Image on Pulga Zephyr

## Summary

- [**First Steps**](https://github.com/caninos-loucos/pulga-zephyr/blob/main/samples/IoTrees_tutorial/tutorial.md#first-steps)
- **Connecting the Firmware to the Network**
    - *In American Tower Connection*
      - [Create Device](https://github.com/caninos-loucos/pulga-zephyr/blob/main/samples/IoTrees_tutorial/tutorial.md#create-device)
      - [Create Filter](https://github.com/caninos-loucos/pulga-zephyr/blob/main/samples/IoTrees_tutorial/tutorial.md#create-filter)
      - [Create Connection](https://github.com/caninos-loucos/pulga-zephyr/blob/main/samples/IoTrees_tutorial/tutorial.md#create-connection)
      - [In Coding](https://github.com/caninos-loucos/pulga-zephyr/blob/main/samples/IoTrees_tutorial/tutorial.md#in-coding) 
    - *Device American Tower screenshot*
        - [Device](https://github.com/caninos-loucos/pulga-zephyr/blob/main/samples/IoTrees_tutorial/tutorial.md#device)
        - [Filter](https://github.com/caninos-loucos/pulga-zephyr/blob/main/samples/IoTrees_tutorial/tutorial.md#filter)
        - [Connection](https://github.com/caninos-loucos/pulga-zephyr/blob/main/samples/IoTrees_tutorial/tutorial.md#connection)
    - [*In Google Sheets*](https://github.com/caninos-loucos/pulga-zephyr/blob/main/samples/IoTrees_tutorial/tutorial.md#in-google-sheets)
      - Google Sheets screenshot
        - [Google Sheets ID](https://github.com/caninos-loucos/pulga-zephyr/blob/main/samples/IoTrees_tutorial/tutorial.md#sheets-id)
        - [Deploy](https://github.com/caninos-loucos/pulga-zephyr/blob/main/samples/IoTrees_tutorial/tutorial.md#deploy-application)
- [**Building an Image**](https://github.com/caninos-loucos/pulga-zephyr/blob/main/samples/IoTrees_tutorial/tutorial.md#building-image)

## First Steps

After completing the [Getting Started](https://github.com/caninos-loucos/pulga-zephyr?tab=readme-ov-file#getting-started) section in README.md:

- First, open a virtual environment and synchronize it in the terminal using the following command:

  ``` console
  cd path/to/where/venv/was/installed && source .venv/bin/activate
  west update
  ```

Now, it is possible to make changes inside the code.

In the `PULGA-ZEPHYR\app` directory:
- Open the `CMakeLists.txt` file:
    - Uncomment the following lines:
        - `list(APPEND SHIELD pulga-lora)`
        - `list(APPEND SHIELD scd30)`

- Open the `prj.config` file:
    - Modify the parameters as follows:
        ```c
        CONFIG_TRANSMISSION_INTERVAL = 300000    // in milliseconds
        CONFIG_SAMPLING_INTERVAL = 1800000       // in milliseconds
        CONFIG_LORAWAN_DR = 0                    // must be set to 0
        ```

> For more details, refer to the [Application](https://github.com/caninos-loucos/pulga-zephyr?tab=readme-ov-file#application-configurations) section in README.md.

---
## Connecting the Firmware to the Network

### In American Tower Conection

#### Create Device

To create a new device in [American Tower Connection](https://ns.atc.everynet.io/login?next=%2Fdevices), follow these steps:
- Go to Devices
    - Create Device
        - Activation = ABP
        - Encryption = NS
        - Class = A
        - Counter size = 4
        - Band = LA915-928A
        - After setting up the configurations above, generate a random number for:
            - Device EUI `64-bit`
            - Application EUI `64-bit`
            - Device address `32-bit`
            - Network session key `128-bit`
            - Application session key `128-bit`
            - And choose a tag to *Tags* field
For more details, see [Images](https://github.com/caninos-loucos/pulga-zephyr/blob/main/samples/IoTrees_tutorial/tutorial.md#american-tower-screenshot) below.

#### Create Filter

- Device tags &rarr; **Choose a tag, must to be the same that you put in Device section**
- Message types &rarr; Select only `uplink`
For more details, see [Images]() below.

#### Create Connection

- Select `HTTP v2` 
- Filter &rarr; Use the key from the filter section.
- Application URL &rarr; Enter the Google Sheets link.  
    - See the [Google Sheets](https://github.com/caninos-loucos/pulga-zephyr/blob/main/samples/IoTrees_tutorial/tutorial.md#in-google-sheets) section below for more details.
---
### In Coding 

Path: `PULGA-ZEPHY/app/comunicaition/lorawan/lorawan_buffer/lorawan_keys_exampla.h`
- Here, the code must to match with the same number in the site

    - Device EUI = `LORAWAN_DEV_EUI` 
    - Application EUI = `LORAWAN_APP_EUI`
    - Device address = `LORAWAN_DEV_ADDR`
    - Network session key =`LORAWAN_NET_KEY`
    - Application session key = `LORAWAN_APP_KEY`

``` c
//The "0x" i's a indcation that the number will be read as binary

#define LORAWAN_DEV_EUI {0x70, 0xc4, 0xde, 0x69, 0xf0, 0xa1, 0x1c, 0x40} 

/*
  To convert a hexadecimal key to binary format, follow these steps:
 
  1. Copy the generated hexadecimal key from the site or any key generator.
  2. Paste it here. Example: 70c4de69f0a11c40
  3. Insert commas between each byte: ,70,c4,de,69,f0,a1,1c,40
  4. Use "Find & Replace" to replace "," with ", 0x".
  5. The final result should look like this:
 
     0x70, 0xc4, 0xde, 0x69, 0xf0, 0xa1, 0x1c, 0x40
 
  6. Repeat these steps for all other keys.
 */


#define LORAWAN_APP_EUI {0x95, 0x20, 0x86, 0x94, 0xe4, 0xc1, 0x68, 0xa7}

#define LORAWAN_APP_KEY {0xb7, 0xca, 0x29, 0xa9, 0x02, 0xcc, 0x38, 0x13, 0x9c, 0x8d, 0x2d, 0x86, 0x06, 0x42, 0xf2, 0x1b}


#define LORAWAN_NET_KEY {0x1e, 0x91, 0xcd, 0xec, 0xad, 0xe1, 0x68, 0x95, 0xc8, 0x54, 0xfe, 0x28, 0xe4, 0xdc, 0xeb, 0x78}


#define LORAWAN_DEV_ADDR 0xd96f77df

```

### American Tower screenshot

#### Device
<div align="center">

![Device part 1](https://github.com/caninos-loucos/pulga-zephyr/blob/main/samples/IoTrees_tutorial/images/createdevicefirststep.jpeg)

</div>

<div align="center">

![Device part 2](https://github.com/caninos-loucos/pulga-zephyr/blob/main/samples/IoTrees_tutorial/images/createdevicesecondstep.jpeg)

</div>

#### Filter 
<div align="center">

![Filter](https://github.com/caninos-loucos/pulga-zephyr/blob/main/samples/IoTrees_tutorial/images/createfilter.jpeg)

</div>

#### Connection
<div align="center">

![Connection](https://github.com/caninos-loucos/pulga-zephyr/blob/main/samples/IoTrees_tutorial/images/createconnection.jpeg)

</div>

### In Google Sheets

- Path:
    - Extensions > Apps Script
- To configure the application connection, follow these steps:
  - Copy the code below and modify it as described here:
    - Google Sheet using Id: 
        - Example
            - docs.google.com/spreadsheets/d/<mark>1sO6Xqce1NKMzsS28SWCquOBqy7wZhiUHdqbM6FaCgrY </mark>/edit?gid=0#gid=0

For more details, see [Images](https://github.com/caninos-loucos/pulga-zephyr/blob/main/samples/IoTrees_tutorial/tutorial.md#google-sheets-screenshot) below.


``` c

function doPost(request){
    /* Open Google Sheet using ID
    Copy the higlighted part on URL adress of your Google Sheets and inside the openById function
    as the example:
    -> http//docs.google.com/spreadsheets/d/1sO6Xqce1NKMzsS28SWCquOBqy7wZhiUHdqbM6FaCgrY/edit?gid=0#gid=0
    -> 1sO6Xqce1NKMzsS28SWCquOBqy7wZhiUHdqbM6FaCgrY
    -> SpreadsheetApp.openById("1sO6Xqce1NKMzsS28SWCquOBqy7wZhiUHdqbM6FaCgrY")
    */
  var sheet = SpreadsheetApp.openById("1sO6Xqce1NKMzsS28SWCquOBqy7wZhiUHdqbM6FaCgrY").getActiveSheet();
  var result = {"status": "SUCCESS"};
  try{
    // Parse request data
    var data = JSON.parse(request.postData.contents);
    var decoded = Utilities.base64Decode(data.params.payload, Utilities.Charset.UTF_8);
    var payloadStr = Utilities.newBlob(decoded).getDataAsString();

    // Define column mapping - 2 characters indicators come first so it doesn't
    // match the wrong column when processing the payload
    var columnMap = {
      "T": "Temperature", "P": "Pressure", "H": "Humidity",
      "AC": "Acceleration", "R": "Rotation", "LT": "Latitude",
      "LG": "Longitude", "B": "Bearing Angle", "S": "Speed",
      "AL": "Altitude", "TS": "GNSS Time", "D": "GNSS Date",
      "L": "Luminosity", "IR": "Infrared", "UV": "UV Intensity",
      "I": "UV Index", "CO2": "CO2 Concentration"
    };

    // Time the LoRaWAN gateway processed the data
    // Use formattedDate and formattedHour to identify rows uniquely
    var gatewayTime = data.meta.time;
    var gatewayDate = new Date(gatewayTime * 1000);
    var formattedDate = Utilities.formatDate(gatewayDate, "GMT-3:00", "dd-MM-yyyy");
    var formattedHour = Utilities.formatDate(gatewayDate, "GMT-3:00", "HH:mm:ss");
    // Gets sheet headers
    var headers = sheet.getRange(1, 1, 1, sheet.getLastColumn()).getValues()[0];

    // Function to find row where to insert data
    function findInsertingRow(formattedDate, formattedHour) {
      // Gets last row with data
      // Gets indexes of Date and Time columns
      var dateColumn = headers.indexOf("Date");
      var timeColumn = headers.indexOf("Time");

      // Gets data values
      var dataValues = sheet.getDataRange().getDisplayValues();
      // Tries to find existing row with matching date and time
      // Start from last row with data, tries to find in last 10 rows, excluding headers
      for (var i = dataValues.length - 1; i > 0 && i > dataValues.length - 10; i--) {
          var existingDate = dataValues[i][dateColumn];
          var existingHour = dataValues[i][timeColumn];
          //sheet.appendRow([i, existingDate, existingHour, formattedDate, formattedHour]);
          if (!existingDate.localeCompare(formattedDate) && !existingHour.localeCompare(formattedHour)) {
            return i + 1; // Return row index (Google Sheets rows start at 1)
          }
      }
      
      // Adds new row if not found
      var lastRow = sheet.getLastRow() + 1;
      sheet.getRange(lastRow, dateColumn + 1).setValue(formattedDate);
      sheet.getRange(lastRow, timeColumn + 1).setValue(formattedHour);
      return lastRow;
    }

    var row = findInsertingRow(formattedDate, formattedHour);

    // Iterate over known indicators in the payload 
    Object.keys(columnMap).forEach(function(indicator) {
      var regex = new RegExp(indicator + "(\\d+(?:\\.\\d+)?)", "g");
      // Update row with payload data
      var matches;
      while ((matches = regex.exec(payloadStr)) !== null) {
        var value = matches[1].replace(".", ","); // Get numeric value
        var column = headers.indexOf(columnMap[indicator]) + 1;
        if (column > 0) {
          var cell = sheet.getRange(row, column);
          if (cell.isBlank()) {
            cell.setValue(value);
          }
        }
      }
    });

  }catch(exc){
    // If error occurs, throw exception
    result = {"status": "FAILED", "message": exc};
  }

  // Return result
  return ContentService
  .createTextOutput(JSON.stringify(result))
  .setMimeType(ContentService.MimeType.JSON);
}

```
### Google Sheets screenshot

#### Sheets Id
<div align="center">

![Get Sheets ID](https://github.com/caninos-loucos/pulga-zephyr/blob/main/samples/IoTrees_tutorial/images/SheetsID.jpeg)

</div>

#### Deploy Application
<div align="center">

![Deploy Parte 1](https://github.com/caninos-loucos/pulga-zephyr/blob/main/samples/IoTrees_tutorial/images/Deploy_part_1.jpeg)

</div>

<div align="center">

![Deploy Parte 2](https://github.com/caninos-loucos/pulga-zephyr/blob/main/samples/IoTrees_tutorial/images/Deploy_part_2.jpeg)

</div>

<div align="center">

![Deploy Parte 3](https://github.com/caninos-loucos/pulga-zephyr/blob/main/samples/IoTrees_tutorial/images/Deploy_part_3.jpeg)

</div>

<div align="center">

![Deploy Parte 4](https://github.com/caninos-loucos/pulga-zephyr/blob/main/samples/IoTrees_tutorial/images/Deploy_part_4.jpeg)

</div>

<div align="center">

![Deploy Parte 5](https://github.com/caninos-loucos/pulga-zephyr/blob/main/samples/IoTrees_tutorial/images/Deploy_part_5.jpeg)

</div>

_______
## Building Image

To build the image, just follow the steps described in the [Building](https://github.com/caninos-loucos/pulga-zephyr?tab=readme-ov-file#building-and-running) section.


