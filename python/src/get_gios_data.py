#!/usr/bin/python3
# -*- coding: utf-8 -*-

import requests
import json

STATIONID = 622


getStationId = requests.get(
    'http://api.gios.gov.pl/pjp-api/rest/station/sensors/{stationId}'.format(stationId=STATIONID))

sensorId = getStationId.json()[0]['id']

getSensorData = requests.get(
    'http://api.gios.gov.pl/pjp-api/rest/data/getData/{sensorId}'.format(sensorId=sensorId))


getIndex = requests.get(
    'http://api.gios.gov.pl/pjp-api/rest/aqindex/getIndex/{stationId}'.format(stationId=STATIONID))


print(getSensorData.json())
print(getIndex.json())