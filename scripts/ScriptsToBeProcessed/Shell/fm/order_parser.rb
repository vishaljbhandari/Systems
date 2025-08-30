require File.expand_path(File.dirname(__FILE__)) + '/../../wsdl/serverfiles/FMSCheckStatus.rb'
require File.expand_path(File.dirname(__FILE__)) + '/order_response.rb'
require  RAILS_ROOT + '/../src/lib/helpers/utility'
require  RAILS_ROOT + '/../src/lib/framework/inline_wrapper'
require  RAILS_ROOT + '/../src/app/models/field_config'
require  RAILS_ROOT + '/lib/patches/model_perf_patches'
require  RAILS_ROOT + '/../src/lib/performance/gettext_patch'

OPTUS_NETWORK_ID 	= "1024"
VMA_NETWORK_ID 		= "1025"
PHONE_PREFIX		= "61"
class NilClass
   def method_missing(symbol, *args)
       return nil
   end
end
class SOAP::Mapping::Object
   def method_missing(symbol, *args)
       return nil
   end
end
class OptusOrderParser
	include Utility
		attr_accessor :field_data
		attr_accessor :startTime
		attr_accessor :endTime
		def initialize
			reset
		end

	private

		def reset
				if @field_data.nil?
					@field_data = Hash.new
				else
					@field_data.clear
				end

				if @header_info.nil?
					@header_info = Hash.new
				else
					@header_info.clear
				end

				@header_info[:record_config_id] = 160
				@header_info[:field_list_type] = 1
				@startTime = Time.now
		end

		def validate_nil_or_blank(field_name, field_value)
			raise FMSBusinessLogicException.new("invalid #{field_name} (#{field_value})") if field_value.nil? or field_value.blank?
		end
		
		def populate_field_data(key,value)
				@field_data[key] = (value.nil? or value.eql?(NilClass) or value.eql?(SOAP::Mapping::Object)) ? "" : value
		end

		def populate_partyidentification_fields(party_identification)
				case party_identification.identificationType
					when "D"
							populate_field_data("Drivers Licence #",party_identification.documentNumber)		
							populate_field_data("State of Issue",party_identification.issuingState)	
					when "P"
							populate_field_data("Passport #",party_identification.documentNumber)		
							populate_field_data("Country of Issue",party_identification.issuingCountry)
				end
		end

		def handle_arrays(field_name, index)	
    		if field_name.is_a?(Array)
        	return field_name[index]	
				else
					return (index.eql?(0)? field_name : nil )
    		end
		end

public
		
		def orderParser(fMSStatusRequestParam)
				begin
					reset
					log(Logger::INFO, "Processing Order record for #{@startTime.strftime("%d/%m/%Y %H:%M:%S")}")
					log(Logger::INFO, "=============================================================")

					validate_nil_or_blank("requestId", fMSStatusRequestParam.requestId)
					log(Logger::INFO, "Application # : #{fMSStatusRequestParam.requestId}")
					validate_nil_or_blank("callerID", fMSStatusRequestParam.callerId)
					log(Logger::INFO, "Network : #{fMSStatusRequestParam.callerId}")
					validate_nil_or_blank("callerDateTime", fMSStatusRequestParam.callerDateTime)
					log(Logger::INFO, "Application Date : #{fMSStatusRequestParam.callerDateTime}")
				#	validate_nil_or_blank("customer", fMSStatusRequestParam.customer)
				#	validate_nil_or_blank("customer names", fMSStatusRequestParam.customer.name)
					
					raise FMSBusinessLogicException.new("Invalid caller id. Caller id should be 1 or 2") if  fMSStatusRequestParam.callerId.to_i != 1 and fMSStatusRequestParam.callerId.to_i != 2
					
			#		if  handle_arrays(fMSStatusRequestParam.customer.name,0).givenNames.blank? 
			#				raise "ERROR : Invalid givenNames :#{handle_arrays(fMSStatusRequestParam.customer.name,0).givenNames}"
			#		elsif handle_arrays(fMSStatusRequestParam.customer.name,0).familyNames.blank? 
			#				raise "ERROR : Invalid familyNames :#{handle_arrays(fMSStatusRequestParam.customer.name,0).familyNames}"
			#		end

					log(Logger::INFO, "INFO : Mandatory fields are proper")
					log(Logger::INFO, "=============================================================")

					populate_field_data("Not Applicable 10", fMSStatusRequestParam.callerId.to_i == 1 ? OPTUS_NETWORK_ID : VMA_NETWORK_ID) 
					populate_field_data("Not Applicable 11", fMSStatusRequestParam.fraudProfile)
					populate_field_data("Application #", fMSStatusRequestParam.requestId)
					populate_field_data("Application Date", fMSStatusRequestParam.callerDateTime.strftime("%Y/%m/%d %H:%M:%S"))
					populate_field_data("IP Address", fMSStatusRequestParam.callerIpAddress)
					populate_field_data("Market", fMSStatusRequestParam.applicationType)

					total_number_of_services = 0
					for i in 1 .. 10 
							new_type, new_count, existing_type, existing_count = ""							

							if !fMSStatusRequestParam.requestedServices.nil?
								new_type = fMSStatusRequestParam.requestedServices[i-1].serviceName unless fMSStatusRequestParam.requestedServices[i-1].nil?
								new_count =  fMSStatusRequestParam.requestedServices[i-1].serviceNumber unless fMSStatusRequestParam.requestedServices[i-1].nil?
								existing_type = fMSStatusRequestParam.existingServices[i-1].serviceName unless fMSStatusRequestParam.existingServices[i-1].nil?
								existing_count = fMSStatusRequestParam.existingServices[i-1].serviceNumber unless fMSStatusRequestParam.existingServices[i-1].nil?
							end 
							total_number_of_services += new_count.to_i unless new_count.nil?
							total_number_of_services += existing_count.to_i unless existing_count.nil?
							populate_field_data("New Type #{i}", new_type)
							populate_field_data("New Count  #{i}", new_count)
							populate_field_data("Existing Type #{i}", existing_type)
							populate_field_data("Existing Count #{i}", existing_count)
					end 

					populate_field_data("No of Services", total_number_of_services)
					populate_field_data("Dealer Code", fMSStatusRequestParam.dealerCode)
					populate_field_data("Dealer Channel", fMSStatusRequestParam.dealerChannel)
					populate_field_data("Airtime Connect Flag", fMSStatusRequestParam.airTimeConnectFlag)
					populate_field_data("First Name", handle_arrays(fMSStatusRequestParam.customer.name,0).givenNames)
					populate_field_data("Middle Name", handle_arrays(fMSStatusRequestParam.customer.name,0).middleNames)
					populate_field_data("Surname", handle_arrays(fMSStatusRequestParam.customer.name,0).familyNames)
					start_date_time = fMSStatusRequestParam.customer.aliveDuring.startDateTime.strftime("%Y/%m/%d %H:%M:%S")
					now = Time.now
					age = start_date_time.nil? ? "" : now.year	- fMSStatusRequestParam.customer.aliveDuring.startDateTime.year - (fMSStatusRequestParam.customer.aliveDuring.startDateTime.to_time.change(:year => now.year) > now ? 1 : 0)
					populate_field_data("DOB", start_date_time)
					populate_field_data("Age", age)
					
					if fMSStatusRequestParam.customer.partyIdentification.class.eql?(Array)	
						fMSStatusRequestParam.customer.partyIdentification.each do |party_identification|	
							populate_partyidentification_fields(party_identification)
						end
					else
						populate_partyidentification_fields(fMSStatusRequestParam.customer.partyIdentification)
					end unless fMSStatusRequestParam.customer.partyIdentification.nil?
						
						#			@field_data["Not Applicable 12"] = fMSStatusRequestParam.customer.partyIdentification[2].nil? ? "" : fMSStatusRequestParam.customer.partyIdentification[2].issuingCountry
					populate_field_data("Not Applicable 12","")
					mobilePhone = fMSStatusRequestParam.customer.mobilePhone.number.nil? ? "" : fMSStatusRequestParam.customer.mobilePhone.number
					if ( mobilePhone[0,1].to_s == "0" )
							mobilePhone = "+" + mobilePhone.sub("0",PHONE_PREFIX)
					elsif ( mobilePhone[0,2].to_s != "61" )
							mobilePhone = "+" + PHONE_PREFIX + mobilePhone	
					else
							mobilePhone = "+" + mobilePhone
					end unless (mobilePhone.nil? or mobilePhone.empty?)
					populate_field_data("Mobile #",mobilePhone)

					home_ph_area_code = fMSStatusRequestParam.customer.homePhone.nil? ? "" : fMSStatusRequestParam.customer.homePhone.areaCode
					home_phone_number = fMSStatusRequestParam.customer.homePhone.nil? ? "" : fMSStatusRequestParam.customer.homePhone.number
					office_ph_area_code = fMSStatusRequestParam.customer.workPhone.nil? ? "" : fMSStatusRequestParam.customer.workPhone.areaCode
					office_phone_number = fMSStatusRequestParam.customer.workPhone.nil? ? "" : fMSStatusRequestParam.customer.workPhone.number

				#Replacing prefix 0 by +61
				if ( !home_phone_number.nil? and !home_phone_number.empty?)
					if ( !home_ph_area_code.nil? and !home_ph_area_code.empty?)
						if ( home_ph_area_code[0,1].to_s == "0" )
							home_ph_area_code = home_ph_area_code.sub("0",PHONE_PREFIX)
						else
							home_ph_area_code = PHONE_PREFIX + home_ph_area_code 
						end	
						@field_data["Primary/Home #"] = "+" + home_ph_area_code + home_phone_number 
					elsif ( home_phone_number[0,1].to_s == "0" )
						home_phone_number = home_phone_number.sub("0","")
						home_ph_area_code = PHONE_PREFIX
						@field_data["Primary/Home #"] = "+" + home_ph_area_code + home_phone_number 
					else
						home_ph_area_code = PHONE_PREFIX
						@field_data["Primary/Home #"] = "+" + home_ph_area_code + home_phone_number 
					end
				else
						@field_data["Primary/Home #"] = ""
				end

				if ( !office_phone_number.nil? and !office_phone_number.empty?)
					if (!office_ph_area_code.nil? and !office_ph_area_code.empty?)
						if ( office_ph_area_code[0,1].to_s == "0" )
							office_ph_area_code = office_ph_area_code.sub("0",PHONE_PREFIX)
						else
							office_ph_area_code = PHONE_PREFIX + office_ph_area_code 
						end	
						@field_data["Secondary/Work #"] = "+" + office_ph_area_code + office_phone_number 
					elsif ( office_phone_number[0,1].to_s == "0" )
						office_phone_number = office_phone_number.sub("0","")
						office_ph_area_code = PHONE_PREFIX
						@field_data["Secondary/Work #"] = "+" + office_ph_area_code + office_phone_number 
					else
                        office_ph_area_code = PHONE_PREFIX
                        @field_data["Secondary/Work #"] = "+" + office_ph_area_code + office_phone_number 
					end
				else
						@field_data["Secondary/Work #"] = ""
				end
			

				populate_field_data("Current Unit",fMSStatusRequestParam.customer.currentAddress.flatUnit.number)
				populate_field_data("Current St #",fMSStatusRequestParam.customer.currentAddress.houseNumber1)
				populate_field_data("Current Street",fMSStatusRequestParam.customer.currentAddress.street.name)
				populate_field_data("Current St Type",fMSStatusRequestParam.customer.currentAddress.street.type)
				populate_field_data("Current Suburb",fMSStatusRequestParam.customer.currentAddress.suburbPlaceLocality)
				populate_field_data("Current State",fMSStatusRequestParam.customer.currentAddress.stateTerritory)
				populate_field_data("Current Post Code",fMSStatusRequestParam.customer.currentAddress.postcode)
				populate_field_data("Current Building",fMSStatusRequestParam.customer.currentAddress.buildingPropertyName)
				populate_field_data("Current Tenure" , fMSStatusRequestParam.residentFromMonths)
				populate_field_data("Current Status" , fMSStatusRequestParam.residentialStatus)
				populate_field_data("Not Applicable 13" , (fMSStatusRequestParam.overSeasAddress.eql?(true) ? 1 : 0))
				populate_field_data("Previous Unit",fMSStatusRequestParam.previousAddress.flatUnit.number)
				populate_field_data("Previous St #",fMSStatusRequestParam.previousAddress.houseNumber1)
				populate_field_data("Previous Street",fMSStatusRequestParam.previousAddress.street.name)
				populate_field_data("Previous St Type",fMSStatusRequestParam.previousAddress.street.type)
				populate_field_data("Previous Suburb",fMSStatusRequestParam.previousAddress.suburbPlaceLocality)
				populate_field_data("Previous State",fMSStatusRequestParam.previousAddress.stateTerritory)
				populate_field_data("Previous Post Code",fMSStatusRequestParam.previousAddress.postcode)
				populate_field_data("Previous Building",fMSStatusRequestParam.previousAddress.buildingPropertyName)
				populate_field_data("Trading Unit",fMSStatusRequestParam.businessDetails.tradingAddress.flatUnit.number)
				populate_field_data("Trading St #",fMSStatusRequestParam.businessDetails.tradingAddress.houseNumber1)
				populate_field_data("Trading St",fMSStatusRequestParam.businessDetails.tradingAddress.street.name)
				populate_field_data("Trading St Type",fMSStatusRequestParam.businessDetails.tradingAddress.street.type)
				populate_field_data("Trading Suburb",fMSStatusRequestParam.businessDetails.tradingAddress.suburbPlaceLocality)
				populate_field_data("Trading State",fMSStatusRequestParam.businessDetails.tradingAddress.stateTerritory)
				populate_field_data("Trading Post Code",fMSStatusRequestParam.businessDetails.tradingAddress.postcode)
				populate_field_data("Trading Building",fMSStatusRequestParam.businessDetails.tradingAddress.buildingPropertyName)
				populate_field_data("Delivery Unit",fMSStatusRequestParam.deliveryAddress.flatUnit.number)
				populate_field_data("Delivery St #",fMSStatusRequestParam.deliveryAddress.houseNumber1)
				populate_field_data("Delivery Street",fMSStatusRequestParam.deliveryAddress.street.name)
				populate_field_data("Delivery St Type",fMSStatusRequestParam.deliveryAddress.street.type)
				populate_field_data("Delivery Suburb",fMSStatusRequestParam.deliveryAddress.suburbPlaceLocality)
				populate_field_data("Delivery State",fMSStatusRequestParam.deliveryAddress.stateTerritory)
				populate_field_data("Delivery Post Code",fMSStatusRequestParam.deliveryAddress.postcode)
				populate_field_data("Email",fMSStatusRequestParam.customer.emailAddress)
				populate_field_data("Billing Unit",fMSStatusRequestParam.billingAddress.flatUnit.number)
				populate_field_data("Not Applicable 14",fMSStatusRequestParam.billingAddress.houseNumber1 )
				populate_field_data("Billing Building",fMSStatusRequestParam.billingAddress.buildingPropertyName)
				populate_field_data("Billing Street",fMSStatusRequestParam.billingAddress.street.name)
				populate_field_data("Billing Street Type",fMSStatusRequestParam.billingAddress.street.type)
				populate_field_data("Billing Suburb",fMSStatusRequestParam.billingAddress.suburbPlaceLocality)
				populate_field_data("Billing State",fMSStatusRequestParam.billingAddress.stateTerritory)
				populate_field_data("Billing Post Code",fMSStatusRequestParam.billingAddress.postcode)
				populate_field_data("Employer",fMSStatusRequestParam.customerCompany)
				populate_field_data("Occupation",fMSStatusRequestParam.customerOccupation)
				populate_field_data("Emp Status",fMSStatusRequestParam.customerEmploymentStatus)
				populate_field_data("Emp Tenure",fMSStatusRequestParam.customerEmployedFrom)
				populate_field_data("ABN",fMSStatusRequestParam.businessDetails.aBN)
				populate_field_data("ACN",fMSStatusRequestParam.businessDetails.aCN)
				populate_field_data("ARBN",fMSStatusRequestParam.businessDetails.aRBN)
				populate_field_data("Business Type",fMSStatusRequestParam.businessDetails.businessType)
				populate_field_data("Business Name",handle_arrays(fMSStatusRequestParam.businessDetails.name,0).tradingName)
				populate_field_data("Years Trading",fMSStatusRequestParam.businessDetails.yearsTrading)
				populate_field_data("# Employees",fMSStatusRequestParam.businessDetails.numberOfEmployees)
				populate_field_data("Credit Card Type",fMSStatusRequestParam.creditCard.cardType)
				populate_field_data("Credit Card #",fMSStatusRequestParam.creditCard.cardNumber)
				expiryMonth  = '00'
				expiryYear = '00'
				expiryMonth = (fMSStatusRequestParam.creditCard.expiryMonth.to_i > 9 ? fMSStatusRequestParam.creditCard.expiryMonth.to_s : ('0' + fMSStatusRequestParam.creditCard.expiryMonth.to_s) ) unless fMSStatusRequestParam.creditCard.expiryMonth.nil?
				expiryYear = (fMSStatusRequestParam.creditCard.expiryYear.to_i > 9 ? fMSStatusRequestParam.creditCard.expiryYear.to_s : ('0' + fMSStatusRequestParam.creditCard.expiryYear.to_s)) unless fMSStatusRequestParam.creditCard.expiryYear.nil?
				populate_field_data("Credit Card Expiry",expiryMonth + expiryYear)
				populate_field_data("Veda NTB", fMSStatusRequestParam.creditReport.vedaCreditDetails.newToBureau.eql?(true) ? 1 : 0)
				populate_field_data("Veda X Ref ", fMSStatusRequestParam.creditReport.vedaCreditDetails.crossreferenceListing.eql?(true) ? 1 : 0)
				populate_field_data("Veda Enq - 1 Mth", fMSStatusRequestParam.creditReport.vedaCreditDetails.enquiriesInLastMonth)
				populate_field_data("Veda Enq - 3 Mth",fMSStatusRequestParam.creditReport.vedaCreditDetails.enquiriesLastThreeMonths)
				populate_field_data("Veda Enq - 6 Mth",fMSStatusRequestParam.creditReport.vedaCreditDetails.enquiriesLastSixMonths)
				populate_field_data("Veda Telco Enq - 1 Mth", fMSStatusRequestParam.creditReport.vedaCreditDetails.telcoEnquiriesLastMonth)
				populate_field_data("Veda Telco Enq - 3 Mth",fMSStatusRequestParam.creditReport.vedaCreditDetails.telcoEnquiriesLastThreeMonths)
				populate_field_data("Veda Telco Enq - 6 Mth",fMSStatusRequestParam.creditReport.vedaCreditDetails.telcoEnquiriesLastSixMonths)
				populate_field_data("Veda # Non-Optus Enq - 6 Mths",fMSStatusRequestParam.creditReport.vedaCreditDetails.nonOptusTelcoEnquiriesLastSixMonths)
				populate_field_data("Veda Total Enq",fMSStatusRequestParam.creditReport.vedaCreditDetails.totalCreditEnquiries)
				populate_field_data("Veda File Date ",fMSStatusRequestParam.creditReport.vedaCreditDetails.creditFileCreateDate.strftime("%Y/%m/%d"))
				populate_field_data("Veda Incorp Date",fMSStatusRequestParam.creditReport.vedaCreditDetails.incorporationDate.strftime("%Y/%m/%d"))
				populate_field_data("Veda Director First Name",handle_arrays(fMSStatusRequestParam.creditReport.vedaCreditDetails.directorsDetails.name,0).givenNames)
				populate_field_data("Veda Director Middle Name",handle_arrays(fMSStatusRequestParam.creditReport.vedaCreditDetails.directorsDetails.name,0).middleNames)
				populate_field_data("Veda Director Surname",handle_arrays(fMSStatusRequestParam.creditReport.vedaCreditDetails.directorsDetails.name,0).familyNames)
				populate_field_data("Veda Director DOB",fMSStatusRequestParam.creditReport.vedaCreditDetails.directorsDetails.aliveDuring.startDateTime.strftime("%Y/%m/%d %H:%M:%S") )
				populate_field_data("Veda Incorp Unit",fMSStatusRequestParam.creditReport.vedaCreditDetails.registeredAddress.flatUnit.number )
				populate_field_data("Veda Incorp St #",fMSStatusRequestParam.creditReport.vedaCreditDetails.registeredAddress.houseNumber1 )
				populate_field_data("Veda Incorp St",fMSStatusRequestParam.creditReport.vedaCreditDetails.registeredAddress.street.name)
				populate_field_data("Veda Incorp Suburb",fMSStatusRequestParam.creditReport.vedaCreditDetails.registeredAddress.suburbPlaceLocality)
				populate_field_data("Veda Incorp State",fMSStatusRequestParam.creditReport.vedaCreditDetails.registeredAddress.stateTerritory)
				populate_field_data("Veda Incorp Post Code",fMSStatusRequestParam.creditReport.vedaCreditDetails.registeredAddress.postcode)
				populate_field_data("Veda File Age",get_age_in_months(fMSStatusRequestParam.creditReport.vedaCreditDetails.creditFileCreateDate))
				populate_field_data("Veda Incorp Age",get_age_in_months(fMSStatusRequestParam.creditReport.vedaCreditDetails.incorporationDate))
				populate_field_data("DnB NTB",fMSStatusRequestParam.creditReport.dunBradCreditDetails.newToBureau.eql?(true) ? 1 : 0)
				populate_field_data("DnB X Ref",fMSStatusRequestParam.creditReport.dunBradCreditDetails.crossreferenceListing.eql?(true) ? 1 : 0)
				populate_field_data("DnB Enq - 1 Mth",fMSStatusRequestParam.creditReport.dunBradCreditDetails.enquiriesInLastMonth)
				populate_field_data("DnB Enq - 3 Mth",fMSStatusRequestParam.creditReport.dunBradCreditDetails.enquiriesLastThreeMonths)
				populate_field_data("DnB Enq - 6 Mth",fMSStatusRequestParam.creditReport.dunBradCreditDetails.enquiriesLastSixMonths)
				populate_field_data("DnB Telco Enq - 1 Mth",fMSStatusRequestParam.creditReport.dunBradCreditDetails.telcoEnquiriesLastMonth)
				populate_field_data("DnB Telco Enq - 3 Mth",fMSStatusRequestParam.creditReport.dunBradCreditDetails.telcoEnquiriesLastThreeMonths)
				populate_field_data("DnB Telco Enq - 6 Mth",fMSStatusRequestParam.creditReport.dunBradCreditDetails.telcoEnquiriesLastSixMonths)
				populate_field_data("DnB # Non-Optus Enq - 6 Mths",fMSStatusRequestParam.creditReport.dunBradCreditDetails.nonOptusTelcoEnquiriesLastSixMonths)
				populate_field_data("DnB Total Enq",fMSStatusRequestParam.creditReport.dunBradCreditDetails.totalCreditEnquiries)
				populate_field_data("DnB File Date",fMSStatusRequestParam.creditReport.dunBradCreditDetails.creditFileCreateDate.strftime("%Y/%m/%d"))
				populate_field_data("DnB Incorp Date",fMSStatusRequestParam.creditReport.dunBradCreditDetails.incorporationDate.strftime("%Y/%m/%d"))
				populate_field_data("DnB Director First Name",handle_arrays(fMSStatusRequestParam.creditReport.dunBradCreditDetails.directorsDetails.name,0).givenNames)
				populate_field_data("DnB Director Middle Name",handle_arrays(fMSStatusRequestParam.creditReport.dunBradCreditDetails.directorsDetails.name,0).middleNames)
				populate_field_data("DnB Director Surname",handle_arrays(fMSStatusRequestParam.creditReport.dunBradCreditDetails.directorsDetails.name,0).familyNames)
				populate_field_data("DnB Director DOB",fMSStatusRequestParam.creditReport.dunBradCreditDetails.directorsDetails.aliveDuring.startDateTime.strftime("%Y/%m/%d %H:%M:%S"))
				populate_field_data("DnB Incorp Unit",fMSStatusRequestParam.creditReport.dunBradCreditDetails.registeredAddress.flatUnit.number)
				populate_field_data("DnB Incorp St #",fMSStatusRequestParam.creditReport.dunBradCreditDetails.registeredAddress.houseNumber1)
				populate_field_data("DnB Incorp St Name",fMSStatusRequestParam.creditReport.dunBradCreditDetails.registeredAddress.street.name)
				populate_field_data("DnB Incorp Suburb",fMSStatusRequestParam.creditReport.dunBradCreditDetails.registeredAddress.suburbPlaceLocality)
				populate_field_data("DnB Incorp State",fMSStatusRequestParam.creditReport.dunBradCreditDetails.registeredAddress.stateTerritory)
				populate_field_data("DnB Incorp Post Code",fMSStatusRequestParam.creditReport.dunBradCreditDetails.registeredAddress.postcode)
				populate_field_data("DnB File Age",get_age_in_months(fMSStatusRequestParam.creditReport.dunBradCreditDetails.creditFileCreateDate))
				populate_field_data("DnB Incorp Age",get_age_in_months(fMSStatusRequestParam.creditReport.dunBradCreditDetails.incorporationDate))
				for i in 1..5
					populate_field_data("Pre Bureau #{i}",fMSStatusRequestParam.creditReport.bureauRules[i-1].preRule)
					populate_field_data("Post Bureau #{i}",fMSStatusRequestParam.creditReport.bureauRules[i-1].postRule)
					populate_field_data("ICC #{i}",fMSStatusRequestParam.creditReport.iccRules[i-1])
				end
				populate_field_data("Credit Decision",fMSStatusRequestParam.creditReport.applicationFinalDecision)
				populate_field_data("Final Credit Score",fMSStatusRequestParam.creditReport.finalCreditScore)
				populate_field_data("Account #",fMSStatusRequestParam.billingAccountNumber)
				populate_field_data("Entity Type",fMSStatusRequestParam.businessDetails.companyType)
				populate_field_data("Account Date",fMSStatusRequestParam.accountActivationDate.strftime("%Y/%m/%d"))
				populate_field_data("Not Applicable 15",fMSStatusRequestParam.mothersMaidenName)
				populate_field_data("Blacklist Source",fMSStatusRequestParam.listType)
				populate_field_data("Applicant Type",fMSStatusRequestParam.customerType)
				populate_field_data("Balance",handle_arrays(fMSStatusRequestParam.creditReport.customerBalance,0))
				populate_field_data("Current $",handle_arrays(fMSStatusRequestParam.creditReport.customerCurrentBalance,0))
				populate_field_data("Applicant Credit Grade",handle_arrays(fMSStatusRequestParam.creditReport.customerCreditGrade,0))
				populate_field_data("30 Days ",handle_arrays(fMSStatusRequestParam.creditReport.accountThirtyDaysArrears,0))
				populate_field_data("60+ Days",handle_arrays(fMSStatusRequestParam.creditReport.accountSixyPlusDaysArrears,0))
				populate_field_data("Paid 6 Mnths",handle_arrays(fMSStatusRequestParam.creditReport.accountTotalPaidLastSixMonths,0))
				populate_field_data("ECC-ICC Response",handle_arrays(fMSStatusRequestParam.creditReport.eccICCResponse,0))
				populate_field_data("Exclusion Flag",handle_arrays(fMSStatusRequestParam.creditReport.exclusionFlag,0).eql?(true) ? 1 : 0)
				populate_field_data("Never Pay Score",handle_arrays(fMSStatusRequestParam.creditReport.neverPayScore,0).to_i)
				populate_field_data("PACMAN Flag",fMSStatusRequestParam.pacManFlag)
				populate_field_data("P2P Migration",fMSStatusRequestParam.isPrePostMigration.eql?(true) ? 1 : 0)
				populate_field_data("Title",handle_arrays(fMSStatusRequestParam.customer.name,0).formOfAddress)
				populate_field_data("Media Code",fMSStatusRequestParam.mediaCode)
				populate_field_data("Supply Partner",fMSStatusRequestParam.supplyPartner)
				populate_field_data("Income",fMSStatusRequestParam.applicantIncome)
				if ( !fMSStatusRequestParam.accountActivationDate.nil? and fMSStatusRequestParam.callerDateTime > fMSStatusRequestParam.accountActivationDate)
					populate_field_data("Account Age",((fMSStatusRequestParam.callerDateTime - fMSStatusRequestParam.accountActivationDate)/30).floor)				
				else
					populate_field_data("Account Age","0")
				end
					
				#Extra Fields
				populate_field_data("Order Number","1")
				populate_field_data("Address Line 1","")
				populate_field_data("Address Line 2","")
				populate_field_data("Address Line 3","")
				populate_field_data("City","")
				populate_field_data("Post Code","")
				populate_field_data("Comments","")
				populate_field_data("Hour Of Day","")
				#field to be filled after inline precheck processing
				populate_field_data("FMS Decision","")
				populate_field_data("Refer Flag","")
				log(Logger::INFO, "Data To Wrapper : #{@field_data.inspect}")
				request_time = Time.now
				error_code,response_msg = InlineWrapper.process_request(@header_info,@field_data)
				response_time = Time.now
				if(error_code.to_s != "0")
				log(Logger::INFO, "Elapsed core time : #{response_time-request_time}::Response from Wrapper Error_code = #{error_code} --Response = #{response_msg.inspect}")
				end
				@endTime = Time.now()
				return OptusOrderResponse.orderResponse(fMSStatusRequestParam.callerId.to_i,@field_data["Application #"],@startTime,@endTime,error_code.to_s,response_msg)
				rescue Exception => e
					log(Logger::ERROR, "#{e.message} - #{e.backtrace.join("\n")}")
 					log(Logger::ERROR, "=============================================================")
					log(Logger::ERROR, "Request object Passed #{fMSStatusRequestParam.to_yaml}")
					@endTime = Time.now()
					return OptusOrderResponse.orderResponse(fMSStatusRequestParam.callerId.to_i,fMSStatusRequestParam.requestId,@startTime,@endTime,"1","")
				end
		end

private

		def get_age_in_months(input_date)
				today = Date.today
				return ( input_date.nil? ? "" : ((today - input_date)/30).floor )
		end

end
