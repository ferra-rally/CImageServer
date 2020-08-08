import requests
import json

def resolve_ua(ua):
	r = requests.get('https://cloud.51degrees.com/api/v4/AQQNX4o8SS5cyqQ62Eg.json?user-agent=' + ua)
	parsed_json = (json.loads(r.text))

	x = parsed_json['device']

	return_string = "{}-{}".format(x['screenpixelswidth'], x['screenpixelsheight'])

	return return_string


#print(resolve_ua("Mozilla/5.0 (Linux; Android 5.1.1; SM-G928X Build/LMY47X) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/47.0.2526.83 Mobile Safari/537.36"))
