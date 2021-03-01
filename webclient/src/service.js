import ky from 'ky';

let prefixUrl = process.env.NODE_ENV === 'production' ? '' : 'http://10.0.0.68'

class Service {
    async getInfo(){
        let res = await ky.get('api/v1/info', {prefixUrl})
        if(!res.ok){
            throw res
        }
        return await res.json()
    }

    async getData(){
        let res = await ky.get('api/v1/data', {prefixUrl})
        if(!res.ok){
            throw res
        }
        return await res.json()
    }

    async getSettings(){
        let res = await ky.get('api/v1/settings', {prefixUrl})
        if(!res.ok){
            throw res
        }
        return await res.json()
    }

    async setSettings(settings){
        let res = await ky.post('api/v1/settings', {json: settings, prefixUrl})
        if(!res.ok){
            throw res
        }
        return await res.json()
    }

}

export default new Service()
